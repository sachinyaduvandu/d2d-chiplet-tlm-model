#include <systemc>
#include <fstream>
#include "common_types.h"
#include "traffic_gen.h"
#include "d2d_bridge.h"
#include "memory_target.h"

int sc_main(int argc, char* argv[]) {
    uint32_t num_packets = 50;
    double injection_interval = 8.0;      // ns
    uint32_t buffer_capacity = 4;
    double link_bandwidth_gbps = 32.0;    // GB/s
    uint32_t packet_size_bytes = 64;      // Bytes

    if (argc >= 6) {
        num_packets = std::stoi(argv[1]);
        injection_interval = std::stod(argv[2]);
        buffer_capacity = std::stoi(argv[3]);
        link_bandwidth_gbps = std::stod(argv[4]);
        packet_size_bytes = std::stoi(argv[5]);
    }

    d2d_model::PerformanceStats::get_instance().reset();

    d2d_model::TrafficGen initiator("Chiplet0_Gen", 0, num_packets, 
                                    sc_core::sc_time(injection_interval, sc_core::SC_NS),
                                    packet_size_bytes);
    
    d2d_model::D2DBridge bridge("D2D_Bridge", 
                                buffer_capacity, 
                                buffer_capacity, 
                                link_bandwidth_gbps, 
                                sc_core::sc_time(5, sc_core::SC_NS));

    d2d_model::MemoryTarget target("Chiplet1_Mem", buffer_capacity, sc_core::sc_time(10, sc_core::SC_NS));

    initiator.initiator_socket.bind(bridge.target_socket);
    bridge.initiator_socket.bind(target.target_socket);

    sc_core::sc_start();

    double total_sim_time_ns = sc_core::sc_time_stamp().to_double() / 1000.0;
    auto& stats = d2d_model::PerformanceStats::get_instance();

    // Derived Architectural KPIs
    double achieved_bw_gbps = (total_sim_time_ns > 0) ? (stats.get_total_bytes() / total_sim_time_ns) : 0.0;
    double offered_load_flits_per_ns = 1.0 / injection_interval;
    double link_utilization_pct = (total_sim_time_ns > 0) ? (stats.get_link_busy_time() / total_sim_time_ns) * 100.0 : 0.0;

    std::ofstream csv_file("results.csv", std::ios::app);
    if (csv_file.is_open()) {
        csv_file << buffer_capacity << ","
                 << link_bandwidth_gbps << ","
                 << packet_size_bytes << ","
                 << injection_interval << ","
                 << offered_load_flits_per_ns << ","
                 << achieved_bw_gbps << ","
                 << stats.get_avg_latency() << ","
                 << stats.get_p95_latency() << ","
                 << bridge.get_stall_count() << ","
                 << link_utilization_pct << "\n";
        csv_file.close();
    }

    return 0;
}
