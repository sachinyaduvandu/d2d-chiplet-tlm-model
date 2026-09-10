#pragma once
#include <systemc>
#include <tlm>
#include <vector>
#include <numeric>
#include <algorithm>

namespace d2d_model {

struct PacketExtension : public tlm::tlm_extension<PacketExtension> {
    uint64_t packet_id;
    uint32_t src_chiplet_id;
    uint32_t dest_chiplet_id;
    sc_core::sc_time inject_time;
    sc_core::sc_time bridge_entry_time;
    uint32_t payload_bytes;

    PacketExtension() 
        : packet_id(0), src_chiplet_id(0), dest_chiplet_id(0), 
          inject_time(sc_core::SC_ZERO_TIME), 
          bridge_entry_time(sc_core::SC_ZERO_TIME),
          payload_bytes(64) {}

    virtual tlm_extension_base* clone() const override {
        PacketExtension* ext = new PacketExtension();
        ext->packet_id = this->packet_id;
        ext->src_chiplet_id = this->src_chiplet_id;
        ext->dest_chiplet_id = this->dest_chiplet_id;
        ext->inject_time = this->inject_time;
        ext->bridge_entry_time = this->bridge_entry_time;
        ext->payload_bytes = this->payload_bytes;
        return ext;
    }

    virtual void copy_from(tlm_extension_base const &ext) override {
        const PacketExtension& from = static_cast<const PacketExtension&>(ext);
        this->packet_id = from.packet_id;
        this->src_chiplet_id = from.src_chiplet_id;
        this->dest_chiplet_id = from.dest_chiplet_id;
        this->inject_time = from.inject_time;
        this->bridge_entry_time = from.bridge_entry_time;
        this->payload_bytes = from.payload_bytes;
    }
};

DECLARE_EXTENDED_PHASE(CREDIT_RETURN);

// Performance aggregation tracker
class PerformanceStats {
public:
    static PerformanceStats& get_instance() {
        static PerformanceStats instance;
        return instance;
    }

    void record_latency(double lat_ns) {
        latencies.push_back(lat_ns);
    }

    void add_bytes_received(uint64_t bytes) {
        total_bytes += bytes;
    }

    void record_link_busy_time(double duration_ns) {
        total_link_busy_ns += duration_ns;
    }

    void reset() {
        latencies.clear();
        total_bytes = 0;
        total_link_busy_ns = 0.0;
    }

    double get_avg_latency() const {
        if (latencies.empty()) return 0.0;
        double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
        return sum / latencies.size();
    }

    double get_p95_latency() {
        if (latencies.empty()) return 0.0;
        std::vector<double> sorted = latencies;
        std::sort(sorted.begin(), sorted.end());
        size_t idx = static_cast<size_t>(0.95 * sorted.size());
        return sorted[std::min(idx, sorted.size() - 1)];
    }

    uint64_t get_total_bytes() const { return total_bytes; }
    double get_link_busy_time() const { return total_link_busy_ns; }

private:
    std::vector<double> latencies;
    uint64_t total_bytes = 0;
    double total_link_busy_ns = 0.0;
};

} // namespace d2d_model
