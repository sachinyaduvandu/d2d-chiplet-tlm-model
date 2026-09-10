#pragma once
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include "common_types.h"

namespace d2d_model {

class MemoryTarget : public sc_core::sc_module {
public:
    tlm_utils::simple_target_socket<MemoryTarget> target_socket;

    MemoryTarget(sc_core::sc_module_name name, sc_core::sc_time access_delay = sc_core::sc_time(20, sc_core::SC_NS))
        : sc_core::sc_module(name), target_socket("target_socket"), m_access_delay(access_delay) {
        target_socket.register_nb_transport_fw(this, &MemoryTarget::nb_transport_fw);
    }

    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay) {
        PacketExtension* ext = nullptr;
        trans.get_extension(ext);

        sc_core::sc_time finish_time = sc_core::sc_time_stamp() + delay + m_access_delay;

        if (ext) {
            sc_core::sc_time total_latency = finish_time - ext->inject_time;
            std::cout << "[TARGET @" << finish_time << "] Completed Packet ID: " 
                      << ext->packet_id << " from Chiplet " << ext->src_chiplet_id
                      << " | Total Latency: " << total_latency << std::endl;
        }

        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        return tlm::TLM_COMPLETED;
    }

private:
    sc_core::sc_time m_access_delay;
};

} // namespace d2d_model
