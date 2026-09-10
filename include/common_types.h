#pragma once
#include <systemc>
#include <tlm>
#include <vector>
#include <numeric>
#include <algorithm>
#include <map>

namespace d2d_model {

enum class StreamPriority {
    HIGH = 0,
    LOW = 1
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

// Multi-Target Topology Stats Tracker
class TopologyStats {
public:
    static TopologyStats& get_instance() {
        static TopologyStats instance;
        return instance;
    }

    void record_delivery(uint32_t dest_id, double latency_ns, uint64_t bytes) {
        dest_latencies[dest_id].push_back(latency_ns);
        dest_bytes[dest_id] += bytes;
    }

    void reset() {
        dest_latencies.clear();
        dest_bytes.clear();
    }

    double get_avg_latency(uint32_t dest_id) {
        auto& lat = dest_latencies[dest_id];
        if (lat.empty()) return 0.0;
        return std::accumulate(lat.begin(), lat.end(), 0.0) / lat.size();
    }

    uint64_t get_bytes(uint32_t dest_id) {
        return dest_bytes[dest_id];
    }

private:
    std::map<uint32_t, std::vector<double>> dest_latencies;
    std::map<uint32_t, uint64_t> dest_bytes;
};

} // namespace d2d_model
