#include <systemc>
#include <fstream>
#include <iostream>
#include "common_types.h"
#include "traffic_gen.h"
#include "d2d_bridge.h"
#include "memory_target.h"

int sc_main(int argc, char* argv[]) {
    std::string policy_str = "RR";
    double cpu_interval = 6.0; // ns
    double dma_interval = 2.0; // ns (aggressive bulk)

    if (argc >= 2) policy_str = argv[1];
    if (argc >= 3) cpu_interval = std::stod(argv[2]);
    if (argc >= 4) dma_interval = std::stod(argv[3]);

    d2d_model::ArbPolicy policy = d2d_model::ArbPolicy::ROUND_ROBIN;
    if (policy_str == "FIFO") policy = d2d_model::ArbPolicy::FIFO;
    else if (policy_str == "PRIO") policy = d2d_model::ArbPolicy::STRICT_PRIORITY;

    d2d_model::MetricsCollector::get_instance().reset();

    // Stream 0: CPU (Latency-Critical, High Priority, 50 packets)
    d2d_model::TrafficGen cpu_gen("CPU_Gen", 0, d2d_model::StreamPriority::HIGH, 50, 
                                  sc_core::sc_time(cpu_interval, sc_core::SC_NS), 0.7, 64);

    // Stream 1: DMA (Bulk, Low Priority, 120 packets)
    d2d_model::TrafficGen dma_gen("DMA_Gen", 1, d2d_model::StreamPriority::LOW, 120, 
                                  sc_core::sc_time(dma_interval, sc_core::SC_NS), 0.3, 64);

    // D2D Router Bridge: Memory @ 64 GB/s, NPU @ 16 GB/s
    d2d_model::D2DRouterBridge bridge("D2D_Router", policy, 8, 8, 64.0, 16.0, sc_core::sc_time(4, sc_core::SC_NS));

    // Target 1: Memory (Fast service 2ns)
    d2d_model::ChipletTarget mem_target("Chiplet1_Memory", 1, 8, sc_core::sc_time(2, sc_core::SC_NS));

    // Target 2: NPU (Slower service 6ns)
    d2d_model::ChipletTarget npu_target("Chiplet2_NPU", 2, 8, sc_core::sc_time(6, sc_core::SC_NS));

    // Bindings
    cpu_gen.initiator_socket.bind(bridge.target_socket_cpu);
    dma_gen.initiator_socket.bind(bridge.target_socket_dma);
    bridge.initiator_socket_mem.bind(mem_target.target_socket);
    bridge.initiator_socket_npu.bind(npu_target.target_socket);

    sc_core::sc_start();

    auto& metrics = d2d_model::MetricsCollector::get_instance();
    double total_sim_time_ns = sc_core::sc_time_stamp().to_seconds() * 1e9;
    double throughput_gbps = (total_sim_time_ns > 0) ? (metrics.get_total_bytes() / total_sim_time_ns) : 0.0;

    std::ofstream csv_file("arbitration_results.csv", std::ios::app);
    if (csv_file.is_open()) {
        csv_file << policy_str << ","
                 << cpu_interval << ","
                 << metrics.get_avg_latency(0) << ","
                 << metrics.get_avg_latency(1) << ","
                 << metrics.get_overall_avg_latency() << ","
                 << metrics.get_p95_latency() << ","
                 << throughput_gbps << ","
                 << metrics.get_avg_queue_occupancy() << ","
                 << metrics.get_max_queue_occupancy() << ","
                 << metrics.get_starvation_events() << ","
                 << metrics.get_jains_fairness() << "\n";
        csv_file.close();
    }

    return 0;
}
