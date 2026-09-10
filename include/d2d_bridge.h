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
    tlm_utils::simple_target_socket<D2DRouterBridge> target_socket_dma;
    tlm_utils::simple_initiator_socket<D2DRouterBridge> initiator_socket_mem; // Chiplet 1
    tlm_utils::simple_initiator_socket<D2DRouterBridge> initiator_socket_npu; // Chiplet 2

    D2DRouterBridge(sc_core::sc_module_name name,
                    ArbPolicy policy = ArbPolicy::ROUND_ROBIN,
                    uint32_t queue_depth = 8,
                    uint32_t initial_credits = 8,
                    double mem_link_bw_gbps = 64.0,
                    double npu_link_bw_gbps = 16.0,
                    sc_core::sc_time link_delay = sc_core::sc_time(5, sc_core::SC_NS))
        : sc_core::sc_module(name),
          target_socket_cpu("target_socket_cpu"),
          target_socket_dma("target_socket_dma"),
          initiator_socket_mem("initiator_socket_mem"),
          initiator_socket_npu("initiator_socket_npu"),
          m_policy(policy),
          m_capacity(queue_depth),
          m_mem_credits(initial_credits),
          m_npu_credits(initial_credits),
          m_mem_bw(mem_link_bw_gbps),
          m_npu_bw(npu_link_bw_gbps),
          m_link_delay(link_delay),
          m_rr_turn(0)
    {
        target_socket_cpu.register_nb_transport_fw(this, &D2DRouterBridge::nb_transport_fw_cpu);
        target_socket_dma.register_nb_transport_fw(this, &D2DRouterBridge::nb_transport_fw_dma);
        initiator_socket_mem.register_nb_transport_bw(this, &D2DRouterBridge::nb_transport_bw_mem);
        initiator_socket_npu.register_nb_transport_bw(this, &D2DRouterBridge::nb_transport_bw_npu);
        SC_THREAD(arbitration_thread);
    }

    tlm::tlm_sync_enum nb_transport_fw_cpu(tlm::tlm_generic_payload& trans,
                                           tlm::tlm_phase& phase,
                                           sc_core::sc_time& delay) {
        return enqueue_packet(&trans, m_cpu_queue, delay);
    }

    tlm::tlm_sync_enum nb_transport_fw_dma(tlm::tlm_generic_payload& trans,
                                           tlm::tlm_phase& phase,
                                           sc_core::sc_time& delay) {
        return enqueue_packet(&trans, m_dma_queue, delay);
    }

    tlm::tlm_sync_enum enqueue_packet(tlm::tlm_generic_payload* trans, 
                                      std::queue<tlm::tlm_generic_payload*>& stream_q,
                                      sc_core::sc_time& delay) {
        if (m_policy == ArbPolicy::FIFO) {
            if (m_shared_fifo.size() >= m_capacity * 2) {
                trans->set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
                return tlm::TLM_COMPLETED;
            }
            m_shared_fifo.push(trans);
        } else {
            if (stream_q.size() >= m_capacity) {
                trans->set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
                return tlm::TLM_COMPLETED;
            }
            stream_q.push(trans);
        }
        m_item_event.notify(delay);
        trans->set_response_status(tlm::TLM_OK_RESPONSE);
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

    void arbitration_thread() {
        while (true) {
            size_t total_items = (m_policy == ArbPolicy::FIFO) ? 
                m_shared_fifo.size() : (m_cpu_queue.size() + m_dma_queue.size());

            MetricsCollector::get_instance().record_queue_sample(total_items);

            if (total_items == 0) {
                wait(m_item_event);
            }

            tlm::tlm_generic_payload* selected_trans = nullptr;

            if (m_policy == ArbPolicy::FIFO) {
                if (!m_shared_fifo.empty()) {
                    selected_trans = m_shared_fifo.front();
                    bool to_mem = (selected_trans->get_address() < 0x8000);

                    // HOL Blocking: Sits blocked if selected head destination has 0 credits
                    while ((to_mem && m_mem_credits == 0) || (!to_mem && m_npu_credits == 0)) {
                        MetricsCollector::get_instance().record_starvation();
                        wait(m_credit_event);
                    }
                    m_shared_fifo.pop();
                }
            } else if (m_policy == ArbPolicy::STRICT_PRIORITY) {
                // Priority to CPU (High Priority)
                if (!m_cpu_queue.empty()) {
                    selected_trans = m_cpu_queue.front();
                    bool to_mem = (selected_trans->get_address() < 0x8000);
                    if ((to_mem && m_mem_credits > 0) || (!to_mem && m_npu_credits > 0)) {
                        m_cpu_queue.pop();
                    } else {
                        selected_trans = nullptr; // Starved on credits, check if DMA can proceed
                    }
                }
                if (!selected_trans && !m_dma_queue.empty()) {
                    tlm::tlm_generic_payload* dma_cand = m_dma_queue.front();
                    bool to_mem = (dma_cand->get_address() < 0x8000);
                    if ((to_mem && m_mem_credits > 0) || (!to_mem && m_npu_credits > 0)) {
                        selected_trans = dma_cand;
                        m_dma_queue.pop();
                    }
                }
                if (!selected_trans) {
                    MetricsCollector::get_instance().record_starvation();
                    wait(m_credit_event);
                    continue;
                }
            } else { // ROUND_ROBIN
                bool check_cpu_first = (m_rr_turn == 0);
                selected_trans = schedule_rr(check_cpu_first);
                if (!selected_trans) {
                    selected_trans = schedule_rr(!check_cpu_first);
                }
                if (!selected_trans) {
                    MetricsCollector::get_instance().record_starvation();
                    wait(m_credit_event);
                    continue;
                }
            }

            if (selected_trans) {
                bool to_mem = (selected_trans->get_address() < 0x8000);
                double bw = to_mem ? m_mem_bw : m_npu_bw;
                if (to_mem) m_mem_credits--; else m_npu_credits--;

                double ser_ns = static_cast<double>(selected_trans->get_data_length()) / bw;
                wait(sc_core::sc_time(ser_ns, sc_core::SC_NS));

                sc_core::sc_time delay = m_link_delay;
                tlm::tlm_phase phase = tlm::BEGIN_REQ;
                if (to_mem) {
                    initiator_socket_mem->nb_transport_fw(*selected_trans, phase, delay);
                } else {
                    initiator_socket_npu->nb_transport_fw(*selected_trans, phase, delay);
                }
            }
        }
    }

    tlm::tlm_generic_payload* schedule_rr(bool try_cpu) {
        if (try_cpu && !m_cpu_queue.empty()) {
            tlm::tlm_generic_payload* p = m_cpu_queue.front();
            bool to_mem = (p->get_address() < 0x8000);
            if ((to_mem && m_mem_credits > 0) || (!to_mem && m_npu_credits > 0)) {
                m_cpu_queue.pop();
                m_rr_turn = 1;
                return p;
            }
        } else if (!try_cpu && !m_dma_queue.empty()) {
            tlm::tlm_generic_payload* p = m_dma_queue.front();
            bool to_mem = (p->get_address() < 0x8000);
            if ((to_mem && m_mem_credits > 0) || (!to_mem && m_npu_credits > 0)) {
                m_dma_queue.pop();
                m_rr_turn = 0;
                return p;
            }
        }
        return nullptr;
    }

private:
    ArbPolicy m_policy;
    uint32_t m_capacity;
    uint32_t m_mem_credits;
    uint32_t m_npu_credits;
    double m_mem_bw;
    double m_npu_bw;
    sc_core::sc_time m_link_delay;
    uint32_t m_rr_turn;

    std::queue<tlm::tlm_generic_payload*> m_shared_fifo;
    std::queue<tlm::tlm_generic_payload*> m_cpu_queue;
    std::queue<tlm::tlm_generic_payload*> m_dma_queue;

    sc_core::sc_event m_item_event;
    sc_core::sc_event m_credit_event;
};

} // namespace d2d_model
