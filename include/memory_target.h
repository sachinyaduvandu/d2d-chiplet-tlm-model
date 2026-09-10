#pragma once
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <queue>
#include "common_types.h"

namespace d2d_model {

class MemoryTarget : public sc_core::sc_module {
public:
    tlm_utils::simple_target_socket<MemoryTarget> target_socket;

    MemoryTarget(sc_core::sc_module_name name, 
                 uint32_t buffer_depth = 16,
                 sc_core::sc_time service_delay = sc_core::sc_time(2, sc_core::SC_NS))
        : sc_core::sc_module(name), 
          target_socket("target_socket"),
          m_capacity(buffer_depth),
          m_free_slots(buffer_depth),
          m_service_delay(service_delay)
    {
        target_socket.register_nb_transport_fw(this, &MemoryTarget::nb_transport_fw);
        SC_THREAD(service_pipeline);
    }

    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay) {
        if (m_free_slots == 0) {
            trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
            return tlm::TLM_COMPLETED;
        }

        m_free_slots--;
        m_queue.push(&trans);
        m_packet_event.notify(delay);

        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        return tlm::TLM_ACCEPTED;
    }

    void service_pipeline() {
        while (true) {
            if (m_queue.empty()) {
                wait(m_packet_event);
            }

            if (!m_queue.empty()) {
                tlm::tlm_generic_payload* trans = m_queue.front();
                m_queue.pop();

                wait(m_service_delay);

                PacketExtension* ext = nullptr;
                trans->get_extension(ext);
                if (ext) {
                    sc_core::sc_time total_lat = sc_core::sc_time_stamp() - ext->inject_time;
                    StreamStats::get_instance().record_packet(ext->src_stream_id, 
                                                              total_lat.to_double() / 1000.0, 
                                                              ext->payload_bytes);
                }

                m_free_slots++;
                tlm::tlm_phase credit_phase = CREDIT_RETURN;
                sc_core::sc_time bw_delay = sc_core::SC_ZERO_TIME;
                target_socket->nb_transport_bw(*trans, credit_phase, bw_delay);

                delete trans;
            }
        }
    }

private:
    uint32_t m_capacity;
    uint32_t m_free_slots;
    sc_core::sc_time m_service_delay;
    std::queue<tlm::tlm_generic_payload*> m_queue;
    sc_core::sc_event m_packet_event;
};

} // namespace d2d_model
