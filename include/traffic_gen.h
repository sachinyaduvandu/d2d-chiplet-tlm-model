#pragma once
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include "common_types.h"

namespace d2d_model {

class TrafficGen : public sc_core::sc_module {
public:
    tlm_utils::simple_initiator_socket<TrafficGen> initiator_socket;

    TrafficGen(sc_core::sc_module_name name, 
               uint32_t stream_id, 
               StreamPriority priority,
               uint32_t num_packets, 
               sc_core::sc_time injection_interval,
               double memory_target_ratio = 0.5,
               uint32_t packet_size_bytes = 64)
        : sc_core::sc_module(name), 
          initiator_socket("initiator_socket"),
          m_stream_id(stream_id),
          m_priority(priority),
          m_num_packets(num_packets),
          m_injection_interval(injection_interval),
          m_mem_ratio(memory_target_ratio),
          m_packet_size_bytes(packet_size_bytes) {
        SC_THREAD(generate_traffic);
    }

    void generate_traffic() {
        for (uint64_t i = 0; i < m_num_packets; ++i) {
            wait(m_injection_interval);

            tlm::tlm_generic_payload* trans = new tlm::tlm_generic_payload();
            PacketExtension* ext = new PacketExtension();
            ext->packet_id = i;
            ext->src_stream_id = m_stream_id;
            ext->priority = m_priority;
            ext->inject_time = sc_core::sc_time_stamp();
            ext->payload_bytes = m_packet_size_bytes;

            // Route between Memory (< 0x8000) and NPU (>= 0x8000)
            bool to_mem = ((double)rand() / RAND_MAX) < m_mem_ratio;
            uint64_t addr = to_mem ? (0x1000 + (i % 64) * 64) : (0x9000 + (i % 64) * 64);
            ext->dest_chiplet_id = to_mem ? 1 : 2;

            trans->set_extension(ext);
            trans->set_command(tlm::TLM_WRITE_COMMAND);
            trans->set_address(addr);
            trans->set_data_length(m_packet_size_bytes);

            sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
            tlm::tlm_phase phase = tlm::BEGIN_REQ;

            while (true) {
                initiator_socket->nb_transport_fw(*trans, phase, delay);
                if (trans->get_response_status() == tlm::TLM_OK_RESPONSE) {
                    break;
                }
                wait(sc_core::sc_time(1, sc_core::SC_NS));
            }
        }
    }

private:
    uint32_t m_stream_id;
    StreamPriority m_priority;
    uint32_t m_num_packets;
    sc_core::sc_time m_injection_interval;
    double m_mem_ratio;
    uint32_t m_packet_size_bytes;
};

} // namespace d2d_model
