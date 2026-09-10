#pragma once
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include "common_types.h"

namespace d2d_model {

class D2DBridge : public sc_core::sc_module {
public:
    tlm_utils::simple_target_socket<D2DBridge> target_socket;
    tlm_utils::simple_initiator_socket<D2DBridge> initiator_socket;

    D2DBridge(sc_core::sc_module_name name, 
              uint32_t buffer_capacity = 2,
              sc_core::sc_time serialization_delay = sc_core::sc_time(8, sc_core::SC_NS),
              sc_core::sc_time link_delay = sc_core::sc_time(12, sc_core::SC_NS))
        : sc_core::sc_module(name),
          target_socket("target_socket"),
          initiator_socket("initiator_socket"),
          m_capacity(buffer_capacity),
          m_credits(buffer_capacity),
          m_serialization_delay(serialization_delay),
          m_link_delay(link_delay),
          m_stall_cycles(0),
          m_fifo(buffer_capacity)
    {
        target_socket.register_nb_transport_fw(this, &D2DBridge::nb_transport_fw);
        SC_THREAD(tx_pipeline_thread);
    }

    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay) {
        if (m_credits == 0) {
            m_stall_cycles++;
            trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
            return tlm::TLM_COMPLETED;
        }

        m_credits--;
        m_fifo.write(&trans);

        PacketExtension* ext = nullptr;
        trans.get_extension(ext);
        if (ext) {
            std::cout << "[D2D_BRIDGE @" << sc_core::sc_time_stamp() << "] Ingested Packet ID: "
                      << ext->packet_id << " | Remaining Credits: " << m_credits << std::endl;
        }

        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        return tlm::TLM_ACCEPTED;
    }

    void tx_pipeline_thread() {
        while (true) {
            tlm::tlm_generic_payload* trans = m_fifo.read();

            // Serialization time over physical lanes
            wait(m_serialization_delay);

            // Forward across die link
            sc_core::sc_time delay = m_link_delay;
            tlm::tlm_phase phase = tlm::BEGIN_REQ;
            initiator_socket->nb_transport_fw(*trans, phase, delay);

            // Replenish credit token
            m_credits++;

            // Wait until link transfer time has passed before destroying payload
            wait(delay);
            delete trans;
        }
    }

    uint64_t get_stall_count() const { return m_stall_cycles; }

private:
    uint32_t m_capacity;
    uint32_t m_credits;
    sc_core::sc_time m_serialization_delay;
    sc_core::sc_time m_link_delay;
    uint64_t m_stall_cycles;
    sc_core::sc_fifo<tlm::tlm_generic_payload*> m_fifo;
};

} // namespace d2d_model
