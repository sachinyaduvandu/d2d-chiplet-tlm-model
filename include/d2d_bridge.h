#pragma once
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <queue>
#include "common_types.h"

namespace d2d_model {

class D2DBridge : public sc_core::sc_module {
public:
    tlm_utils::simple_target_socket<D2DBridge> target_socket;
    tlm_utils::simple_initiator_socket<D2DBridge> initiator_socket;

    D2DBridge(sc_core::sc_module_name name, 
              uint32_t ingress_depth = 4,
              uint32_t initial_downstream_credits = 4,
              double link_bandwidth_gbps = 32.0, // GB/s
              sc_core::sc_time link_delay = sc_core::sc_time(10, sc_core::SC_NS))
        : sc_core::sc_module(name),
          target_socket("target_socket"),
          initiator_socket("initiator_socket"),
          m_ingress_depth(ingress_depth),
          m_ingress_credits(ingress_depth),
          m_downstream_credits(initial_downstream_credits),
          m_link_bandwidth_gbps(link_bandwidth_gbps),
          m_link_delay(link_delay),
          m_stall_cycles(0)
    {
        target_socket.register_nb_transport_fw(this, &D2DBridge::nb_transport_fw);
        initiator_socket.register_nb_transport_bw(this, &D2DBridge::nb_transport_bw);
        SC_THREAD(tx_pipeline_thread);
    }

    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay) {
        if (m_ingress_credits == 0) {
            m_stall_cycles++;
            trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
            return tlm::TLM_COMPLETED;
        }

        m_ingress_credits--;
        m_ingress_queue.push(&trans);
        m_item_pushed_event.notify(delay);

        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        return tlm::TLM_ACCEPTED;
    }

    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay) {
        if (phase == CREDIT_RETURN) {
            m_downstream_credits++;
            m_credit_available_event.notify(delay);
            return tlm::TLM_COMPLETED;
        }
        return tlm::TLM_ACCEPTED;
    }

    void tx_pipeline_thread() {
        while (true) {
            if (m_ingress_queue.empty()) {
                wait(m_item_pushed_event);
            }

            if (!m_ingress_queue.empty()) {
                while (m_downstream_credits == 0) {
                    m_stall_cycles++;
                    wait(m_credit_available_event);
                }

                tlm::tlm_generic_payload* trans = m_ingress_queue.front();
                m_ingress_queue.pop();
                m_downstream_credits--;

                // Architectural Serialization Latency: T_ser = Packet_Size / Bandwidth
                uint32_t packet_bytes = trans->get_data_length();
                double ser_delay_ns = static_cast<double>(packet_bytes) / m_link_bandwidth_gbps;
                sc_core::sc_time serialization_delay(ser_delay_ns, sc_core::SC_NS);

                wait(serialization_delay);
                PerformanceStats::get_instance().record_link_busy_time(ser_delay_ns);

                // Buffer slot freed once serialized onto PHY lanes
                m_ingress_credits++;

                // Transmit flit across link
                sc_core::sc_time delay = m_link_delay;
                tlm::tlm_phase phase = tlm::BEGIN_REQ;
                initiator_socket->nb_transport_fw(*trans, phase, delay);
            }
        }
    }

    uint64_t get_stall_count() const { return m_stall_cycles; }

private:
    uint32_t m_ingress_depth;
    uint32_t m_ingress_credits;
    uint32_t m_downstream_credits;
    double m_link_bandwidth_gbps;
    sc_core::sc_time m_link_delay;
    uint64_t m_stall_cycles;

    std::queue<tlm::tlm_generic_payload*> m_ingress_queue;
    sc_core::sc_event m_item_pushed_event;
    sc_core::sc_event m_credit_available_event;
};

} // namespace d2d_model
