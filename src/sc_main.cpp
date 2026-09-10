#include <systemc>
#include <fstream>
#include "common_types.h"
#include "traffic_gen.h"
#include "d2d_bridge.h"
#include "memory_target.h"

int sc_main(int argc, char* argv[]) {
    // Default parameters
    uint32_t num_packets = 20;
    double injection_interval = 10.0; // ns
    uint32_t buffer_capacity = 4;

    if (argc >= 4) {
        num_packets = std::stoi(argv[1]);
        injection_interval = std::stod(argv[2]);
        buffer_capacity = std::stoi(argv[3]);
    }

    d2d_model::TrafficGen initiator("Chiplet0_Gen", 0, num_packets, sc_core::sc_time(injection_interval, sc_core::SC_NS));
    d2d_model::D2DBridge bridge("D2D_Bridge", buffer_capacity, sc_core::sc_time(8, sc_core::SC_NS), sc_core::sc_time(12, sc_core::SC_NS));
    d2d_model::MemoryTarget target("Chiplet1_Mem", sc_core::sc_time(20, sc_core::SC_NS));

    initiator.initiator_socket.bind(bridge.target_socket);
    bridge.initiator_socket.bind(target.target_socket);

    sc_core::sc_start();

    // Append key metrics to CSV for analysis
    std::ofstream csv_file("results.csv", std::ios::app);
    if (csv_file.is_open()) {
        csv_file << buffer_capacity << ","
                 << injection_interval << ","
                 << num_packets << ","
                 << bridge.get_stall_count() << ","
                 << sc_core::sc_time_stamp().to_double() / 1000.0 << "\n";
        csv_file.close();
    }

    return 0;
}
