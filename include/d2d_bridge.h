#pragma once
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <queue>
#include "common_types.h"

namespace d2d_model {

enum class ArbPolicy {
    ROUND_ROBIN,
    STRICT_PRIORITY
};

class D2DBridge : public sc_core::sc_module {
public:
    tlm_utils::simple_target_socket<D2DBridge> target_socket_cpu;
    tlm_utils::simple_target_socket<D2DBridge> target_socket_dma;
    tlm_utils::simple_initiator_socket<D2DBridge> initiator_socket;

    D2DBridge(sc_core::sc_module_name name, 
              uint32_t ingress_depth_per_queue = 8,
              uint32_t initial_downstream_credits = 16,
              double link_bandwidth_gbps = 32.0,
              sc_core::sc_time link_delay = sc_core::sc_time(5, sc_core::SC_NS),
              ArbPolicy policy = ArbPolicy::ROUND_ROBIN)
        : sc_core::sc_module(name),
          target_socket_cpu("target_socket_cpu"),
          target_socket_dma("target_socket_dma"),
          initiator_socket("initiator_socket"),
          m_capacity(ingress_depth_per_queue),
          m_downstream_credits(initial_downstream_credits),
          m_link_bandwidth_gbps(link_bandwidth_gbps),
          m_link_delay(link_delay),
          m_policy(policy),
          m_rr_turn(0)
    {
        target_socket_cpu.register_nb_transport_fw(this, &D2DBridge::nb_transport_fw_cpu);
        target_socket_dma.register_nb_transport_fw(this, &D2DBridge::nb_transport_fw_dma);
        initiator_socket.register_nb_transport_bw(this, &D2DBridge::nb_transport_bw);
        SC_THREAD(arbitration_thread);
    }

    tlm::tlm_sync_enum nb_transport_fw_cpu(tlm::tlm_generic_payload& trans,
                                           tlm::tlm_phase& phase,
                                           sc_core::sc_time& delay) {
        if (m_hi_queue.size() >= m_capacity) {
            trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
            return tlm::TLM_COMPLETED;
        }
        m_hi_queue.push(&trans);
        m_item_event.notify(delay);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        return tlm::TLM_ACCEPTED;
    }

    tlm::tlm_sync_enum nb_transport_fw_dma(tlm::tlm_generic_payload& trans,
                                           tlm::tlm_phase& phase,
                                           sc_core::sc_time& delay) {
        if (m_lo_queue.size() >= m_capacity) {
            trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
            return tlm::TLM_COMPLETED;
        }
        m_lo_queue.push(&trans);
        m_item_event.notify(delay);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        return tlm::TLM_ACCEPTED;
    }

    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay) {
        if (phase == CREDIT_RETURN) {
            m_downstream_credits++;
            m_credit_event.notify(delay);
            return tlm::TLM_COMPLETED;
        }
        return tlm::TLM_ACCEPTED;
    }

    void arbitration_thread() {
        while (true) {
            if (m_hi_queue.empty() && m_lo_queue.empty()) {
                wait(m_item_event);
            }

            while (m_downstream_credits == 0) {
                wait(m_credit_event);
            }

            tlm::tlm_generic_payload* selected_trans = nullptr;

            if (m_policy == ArbPolicy::STRICT_PRIORITY) {
                if (!m_hi_queue.empty()) {
                    selected_trans = m_hi_queue.front();
                    m_hi_queue.pop();
                } else if (!m_lo_queue.empty()) {
                    selected_trans = m_lo_queue.front();
                    m_lo_queue.pop();
                }
            } else { // ROUND_ROBIN
                if (!m_hi_queue.empty() && !m_lo_queue.empty()) {
                    if (m_rr_turn == 0) {
                        selected_trans = m_hi_queue.front();
                        m_hi_queue.pop();
                        m_rr_turn = 1;
                    } else {
                        selected_trans = m_lo_queue.front();
                        m_lo_queue.pop();
                        m_rr_turn = 0;
                    }
                } else if (!m_hi_queue.empty()) {
                    selected_trans = m_hi_queue.front();
                    m_hi_queue.pop();
                } else if (!m_lo_queue.empty()) {
                    selected_trans = m_lo_queue.front();
                    m_lo_queue.pop();
                }
            }

            if (selected_trans) {
                m_downstream_credits--;

                double ser_ns = static_cast<double>(selected_trans->get_data_length()) / m_link_bandwidth_gbps;
                wait(sc_core::sc_time(ser_ns, sc_core::SC_NS));

                sc_core::sc_time delay = m_link_delay;
                tlm::tlm_phase phase = tlm::BEGIN_REQ;
                initiator_socket->nb_transport_fw(*selected_trans, phase, delay);
            }
        }
    }

private:
    uint32_t m_capacity;
    uint32_t m_downstream_credits;
    double m_link_bandwidth_gbps;
    sc_core::sc_time m_link_delay;
    ArbPolicy m_policy;
    uint32_t m_rr_turn;

    std::queue<tlm::tlm_generic_payload*> m_hi_queue;
    std::queue<tlm::tlm_generic_payload*> m_lo_queue;
    sc_core::sc_event m_item_event;
    sc_core::sc_event m_credit_event;
};

} // namespace d2d_model
