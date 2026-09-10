#pragma once
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <queue>
#include "common_types.h"

namespace d2d_model {

class D2DRouterBridge : public sc_core::sc_module {
public:
    tlm_utils::simple_target_socket<D2DRouterBridge> target_socket_cpu;
    tlm_utils::simple_initiator_socket<D2DRouterBridge> initiator_socket_mem; // Chiplet 1
    tlm_utils::simple_initiator_socket<D2DRouterBridge> initiator_socket_npu; // Chiplet 2

    D2DRouterBridge(sc_core::sc_module_name name,
                    uint32_t ingress_depth = 16,
                    uint32_t initial_credits = 8,
                    double mem_link_bw_gbps = 64.0,
                    double npu_link_bw_gbps = 16.0,
                    sc_core::sc_time link_delay = sc_core::sc_time(5, sc_core::SC_NS))
        : sc_core::sc_module(name),
          target_socket_cpu("target_socket_cpu"),
          initiator_socket_mem("initiator_socket_mem"),
          initiator_socket_npu("initiator_socket_npu"),
          m_capacity(ingress_depth),
          m_mem_credits(initial_credits),
          m_npu_credits(initial_credits),
          m_mem_bw(mem_link_bw_gbps),
          m_npu_bw(npu_link_bw_gbps),
          m_link_delay(link_delay)
    {
        target_socket_cpu.register_nb_transport_fw(this, &D2DRouterBridge::nb_transport_fw);
        initiator_socket_mem.register_nb_transport_bw(this, &D2DRouterBridge::nb_transport_bw_mem);
        initiator_socket_npu.register_nb_transport_bw(this, &D2DRouterBridge::nb_transport_bw_npu);
        
        SC_THREAD(router_thread);
    }

    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay) {
        if (m_queue.size() >= m_capacity) {
            trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
            return tlm::TLM_COMPLETED;
        }

        m_queue.push(&trans);
        m_item_event.notify(delay);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        return tlm::TLM_ACCEPTED;
    }

    tlm::tlm_sync_enum nb_transport_bw_mem(tlm::tlm_generic_payload& trans,
                                           tlm::tlm_phase& phase,
                                           sc_core::sc_time& delay) {
        if (phase == CREDIT_RETURN) {
            m_mem_credits++;
            m_credit_event.notify(delay);
            return tlm::TLM_COMPLETED;
        }
        return tlm::TLM_ACCEPTED;
    }

    tlm::tlm_sync_enum nb_transport_bw_npu(tlm::tlm_generic_payload& trans,
                                           tlm::tlm_phase& phase,
                                           sc_core::sc_time& delay) {
        if (phase == CREDIT_RETURN) {
            m_npu_credits++;
            m_credit_event.notify(delay);
            return tlm::TLM_COMPLETED;
        }
        return tlm::TLM_ACCEPTED;
    }

    void router_thread() {
        while (true) {
            if (m_queue.empty()) {
                wait(m_item_event);
            }

            if (!m_queue.empty()) {
                tlm::tlm_generic_payload* trans = m_queue.front();
                uint64_t addr = trans->get_address();

                // Memory Map Routing Logic:
                // Addr < 0x8000 -> Chiplet 1 (Memory)
                // Addr >= 0x8000 -> Chiplet 2 (NPU)
                bool route_to_mem = (addr < 0x8000);

                if (route_to_mem) {
                    while (m_mem_credits == 0) wait(m_credit_event);
                    m_queue.pop();
                    m_mem_credits--;

                    double ser_ns = static_cast<double>(trans->get_data_length()) / m_mem_bw;
                    wait(sc_core::sc_time(ser_ns, sc_core::SC_NS));

                    sc_core::sc_time delay = m_link_delay;
                    tlm::tlm_phase phase = tlm::BEGIN_REQ;
                    initiator_socket_mem->nb_transport_fw(*trans, phase, delay);
                } else {
                    while (m_npu_credits == 0) wait(m_credit_event);
                    m_queue.pop();
                    m_npu_credits--;

                    double ser_ns = static_cast<double>(trans->get_data_length()) / m_npu_bw;
                    wait(sc_core::sc_time(ser_ns, sc_core::SC_NS));

                    sc_core::sc_time delay = m_link_delay;
                    tlm::tlm_phase phase = tlm::BEGIN_REQ;
                    initiator_socket_npu->nb_transport_fw(*trans, phase, delay);
                }
            }
        }
    }

private:
    uint32_t m_capacity;
    uint32_t m_mem_credits;
    uint32_t m_npu_credits;
    double m_mem_bw;
    double m_npu_bw;
    sc_core::sc_time m_link_delay;

    std::queue<tlm::tlm_generic_payload*> m_queue;
    sc_core::sc_event m_item_event;
    sc_core::sc_event m_credit_event;
};

} // namespace d2d_model
