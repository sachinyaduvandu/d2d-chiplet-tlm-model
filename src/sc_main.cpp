#include <systemc>
#include <fstream>
#include "common_types.h"
#include "traffic_gen.h"
#include "d2d_bridge.h"
#include "memory_target.h"

int sc_main(int argc, char* argv[]) {
    d2d_model::TopologyStats::get_instance().reset();

    // Chiplet 0 Initiator (80 packets total, injected every 3ns)
    d2d_model::TrafficGen cpu_initiator("Chiplet0_CPU", 0, 80, sc_core::sc_time(3, sc_core::SC_NS), 64);

    // Multi-Port Router Bridge (Memory Link: 64 GB/s, NPU Link: 16 GB/s)
    d2d_model::D2DRouterBridge router("D2D_Router", 16, 8, 64.0, 16.0, sc_core::sc_time(5, sc_core::SC_NS));

    // Chiplet 1: Memory Target (Base: 0x0)
    d2d_model::ChipletTarget chiplet1_mem("Chiplet1_Memory", 1, 8, sc_core::sc_time(2, sc_core::SC_NS));

    // Chiplet 2: NPU Target (Base: 0x8000)
    d2d_model::ChipletTarget chiplet2_npu("Chiplet2_NPU", 2, 8, sc_core::sc_time(4, sc_core::SC_NS));

    // Topology Bindings
    cpu_initiator.initiator_socket.bind(router.target_socket_cpu);
    router.initiator_socket_mem.bind(chiplet1_mem.target_socket);
    router.initiator_socket_npu.bind(chiplet2_npu.target_socket);

    sc_core::sc_start();

    auto& stats = d2d_model::TopologyStats::get_instance();
    double total_sim_time = sc_core::sc_time_stamp().to_double() / 1000.0;

    std::ofstream csv_file("topology_results.csv", std::ios::trunc);
    if (csv_file.is_open()) {
        csv_file << "destination,avg_latency_ns,throughput_gbps\n";
        csv_file << "Chiplet1_Memory," << stats.get_avg_latency(1) << "," << (stats.get_bytes(1) / total_sim_time) << "\n";
        csv_file << "Chiplet2_NPU," << stats.get_avg_latency(2) << "," << (stats.get_bytes(2) / total_sim_time) << "\n";
        csv_file.close();
    }

    return 0;
}
