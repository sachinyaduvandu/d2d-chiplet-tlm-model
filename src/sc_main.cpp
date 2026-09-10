#include <systemc>
#include <fstream>
#include "common_types.h"
#include "traffic_gen.h"
#include "d2d_bridge.h"
#include "memory_target.h"

int sc_main(int argc, char* argv[]) {
    std::string policy_str = "RR";
    if (argc >= 2) policy_str = argv[1];

    d2d_model::ArbPolicy policy = (policy_str == "PRIO") ? 
        d2d_model::ArbPolicy::STRICT_PRIORITY : d2d_model::ArbPolicy::ROUND_ROBIN;

    d2d_model::StreamStats::get_instance().reset();

    // CPU Generator: High-priority, periodic latency-critical stream (40 packets, 8ns interval)
    d2d_model::TrafficGen cpu_gen("CPU_Gen", 0, d2d_model::StreamPriority::HIGH, 40, 
                                  sc_core::sc_time(8, sc_core::SC_NS), 64);

    // DMA Generator: Low-priority, aggressive bulk stream (100 packets, 3ns interval)
    d2d_model::TrafficGen dma_gen("DMA_Gen", 1, d2d_model::StreamPriority::LOW, 100, 
                                  sc_core::sc_time(3, sc_core::SC_NS), 64);

    // D2D Bridge with dual dedicated ingress channels and configurable arbiter
    d2d_model::D2DBridge bridge("D2D_Bridge", 8, 16, 32.0, sc_core::sc_time(5, sc_core::SC_NS), policy);

    // Target memory
    d2d_model::MemoryTarget target("Chiplet1_Mem", 16, sc_core::sc_time(2, sc_core::SC_NS));

    // Connect initiators to their dedicated bridge target sockets
    cpu_gen.initiator_socket.bind(bridge.target_socket_cpu);
    dma_gen.initiator_socket.bind(bridge.target_socket_dma);
    bridge.initiator_socket.bind(target.target_socket);

    sc_core::sc_start();

    auto& stats = d2d_model::StreamStats::get_instance();
    double total_sim_time = sc_core::sc_time_stamp().to_double() / 1000.0;

    std::ofstream csv_file("arbitration_results.csv", std::ios::app);
    if (csv_file.is_open()) {
        csv_file << policy_str << ","
                 << stats.get_avg_latency(0) << ","
                 << stats.get_avg_latency(1) << ","
                 << (stats.get_bytes(0) / total_sim_time) << ","
                 << (stats.get_bytes(1) / total_sim_time) << "\n";
        csv_file.close();
    }

    return 0;
}
