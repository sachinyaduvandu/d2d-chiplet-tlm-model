#pragma once
#include <systemc>
#include <tlm>
#include <vector>
#include <numeric>
#include <algorithm>
#include <map>
#include <cmath>

namespace d2d_model {

enum class StreamPriority {
    HIGH = 0,
    LOW = 1
};

enum class ArbPolicy {
    FIFO,
    ROUND_ROBIN,
    STRICT_PRIORITY
};

struct PacketExtension : public tlm::tlm_extension<PacketExtension> {
    uint64_t packet_id;
    uint32_t src_stream_id;
    uint32_t dest_chiplet_id;
    StreamPriority priority;
    sc_core::sc_time inject_time;
    uint32_t payload_bytes;

    PacketExtension() 
        : packet_id(0), src_stream_id(0), dest_chiplet_id(1),
          priority(StreamPriority::LOW),
          inject_time(sc_core::SC_ZERO_TIME), payload_bytes(64) {}

    virtual tlm_extension_base* clone() const override {
        PacketExtension* ext = new PacketExtension();
        ext->packet_id = this->packet_id;
        ext->src_stream_id = this->src_stream_id;
        ext->dest_chiplet_id = this->dest_chiplet_id;
        ext->priority = this->priority;
        ext->inject_time = this->inject_time;
        ext->payload_bytes = this->payload_bytes;
        return ext;
    }

    virtual void copy_from(tlm_extension_base const &ext) override {
        const PacketExtension& from = static_cast<const PacketExtension&>(ext);
        this->packet_id = from.packet_id;
        this->src_stream_id = from.src_stream_id;
        this->dest_chiplet_id = from.dest_chiplet_id;
        this->priority = from.priority;
        this->inject_time = from.inject_time;
        this->payload_bytes = from.payload_bytes;
    }
};

DECLARE_EXTENDED_PHASE(CREDIT_RETURN);

// Unified System Performance & Fairness Metrics Collector
class MetricsCollector {
public:
    static MetricsCollector& get_instance() {
        static MetricsCollector instance;
        return instance;
    }

    void record_delivery(uint32_t stream_id, uint32_t dest_id, double latency_ns, uint64_t bytes) {
        stream_latencies[stream_id].push_back(latency_ns);
        all_latencies.push_back(latency_ns);
        stream_bytes[stream_id] += bytes;
        dest_bytes[dest_id] += bytes;
    }

    void record_starvation() { credit_starvation_events++; }
    
    // Mean sampled occupancy across arbitration loop evaluations
    void record_queue_sample(size_t sz) {
        queue_occupancy_samples.push_back(sz);
        if (sz > max_queue_depth) max_queue_depth = sz;
    }

    void reset() {
        stream_latencies.clear();
        all_latencies.clear();
        stream_bytes.clear();
        dest_bytes.clear();
        queue_occupancy_samples.clear();
        max_queue_depth = 0;
        credit_starvation_events = 0;
    }

    double get_avg_latency(uint32_t stream_id) {
        auto& lat = stream_latencies[stream_id];
        if (lat.empty()) return 0.0;
        return std::accumulate(lat.begin(), lat.end(), 0.0) / lat.size();
    }

    double get_overall_avg_latency() {
        if (all_latencies.empty()) return 0.0;
        return std::accumulate(all_latencies.begin(), all_latencies.end(), 0.0) / all_latencies.size();
    }

    // Precise 0-based P95 calculation
    double get_p95_latency() {
        if (all_latencies.empty()) return 0.0;
        std::vector<double> s = all_latencies;
        std::sort(s.begin(), s.end());
        size_t idx = static_cast<size_t>(std::ceil(0.95 * s.size())) - 1;
        if (idx >= s.size()) idx = s.size() - 1;
        return s[idx];
    }

    uint64_t get_stream_bytes(uint32_t stream_id) { return stream_bytes[stream_id]; }
    uint64_t get_total_bytes() {
        uint64_t total = 0;
        for (auto& kv : stream_bytes) total += kv.second;
        return total;
    }

    // Mean sampled queue occupancy
    double get_mean_sampled_queue_occupancy() {
        if (queue_occupancy_samples.empty()) return 0.0;
        double sum = std::accumulate(queue_occupancy_samples.begin(), queue_occupancy_samples.end(), 0.0);
        return sum / queue_occupancy_samples.size();
    }

    size_t get_max_queue_occupancy() const { return max_queue_depth; }
    uint64_t get_starvation_events() const { return credit_starvation_events; }

    // Jain's Fairness Index across delivered stream volume in finite workload window
    double get_jains_fairness() {
        if (stream_bytes.size() < 2) return 1.0;
        double sum = 0.0;
        double sum_sq = 0.0;
        for (auto& kv : stream_bytes) {
            double x = static_cast<double>(kv.second);
            sum += x;
            sum_sq += (x * x);
        }
        if (sum_sq == 0.0) return 1.0;
        return (sum * sum) / (stream_bytes.size() * sum_sq);
    }

private:
    std::map<uint32_t, std::vector<double>> stream_latencies;
    std::vector<double> all_latencies;
    std::map<uint32_t, uint64_t> stream_bytes;
    std::map<uint32_t, uint64_t> dest_bytes;
    std::vector<size_t> queue_occupancy_samples;
    size_t max_queue_depth = 0;
    uint64_t credit_starvation_events = 0;
};

} // namespace d2d_model
