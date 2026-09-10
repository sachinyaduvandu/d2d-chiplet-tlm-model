#pragma once
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include "common_types.h"

namespace d2d_model {

class TrafficGen : public sc_core::sc_module {
public:
    tlm_utils::simple_initiator_socket<TrafficGen> initiator_socket;

    TrafficGen(sc_core::sc_module_name name, uint32_t chiplet_id, uint32_t num_packets, sc_core::sc_time injection_interval)
        : sc_core::sc_module(name), 
          initiator_socket("initiator_socket"),
          m_chiplet_id(chiplet_id),
          m_num_packets(num_packets),
          m_injection_interval(injection_interval) {
        SC_THREAD(generate_traffic);
    }

    void generate_traffic() {
        for (uint64_t i = 0; i < m_num_packets; ++i) {
            wait(m_injection_interval);

            // Dynamically allocate transaction to ensure persistent lifetime in FIFO queue
            tlm::tlm_generic_payload* trans = new tlm::tlm_generic_payload();
            PacketExtension* ext = new PacketExtension();
            ext->packet_id = i;
            ext->src_chiplet_id = m_chiplet_id;
            ext->dest_chiplet_id = 1; 
            ext->inject_time = sc_core::sc_time_stamp();
            ext->payload_bytes = 64;

            trans->set_extension(ext);
            trans->set_command(tlm::TLM_WRITE_COMMAND);
            trans->set_address(0x1000 + (i * 64));
            trans->set_data_length(64);

            sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
            tlm::tlm_phase phase = tlm::BEGIN_REQ;

            std::cout << "[INITIATOR @" << sc_core::sc_time_stamp() << "] Attempting Injection Packet ID: " << i << std::endl;

            // Handshake with backpressure retry loop
            while (true) {
                initiator_socket->nb_transport_fw(*trans, phase, delay);
                
                if (trans->get_response_status() == tlm::TLM_OK_RESPONSE) {
                    break; // Successfully buffered in bridge
                }

                // Bridge full: wait and retry
                std::cout << "[INITIATOR @" << sc_core::sc_time_stamp() << "] Backpressure! Stalling Packet ID: " << i << std::endl;
                wait(sc_core::sc_time(4, sc_core::SC_NS));
            }
        }
    }

private:
    uint32_t m_chiplet_id;
    uint32_t m_num_packets;
    sc_core::sc_time m_injection_interval;
};

} // namespace d2d_model
