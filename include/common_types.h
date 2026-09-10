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
    StreamPriority priority;
    sc_core::sc_time inject_time;
    uint32_t payload_bytes;

    PacketExtension() 
        : packet_id(0), src_stream_id(0), priority(StreamPriority::LOW),
          inject_time(sc_core::SC_ZERO_TIME), payload_bytes(64) {}

    virtual tlm_extension_base* clone() const override {
        PacketExtension* ext = new PacketExtension();
        ext->packet_id = this->packet_id;
        ext->src_stream_id = this->src_stream_id;
        ext->priority = this->priority;
        ext->inject_time = this->inject_time;
        ext->payload_bytes = this->payload_bytes;
        return ext;
    }

    virtual void copy_from(tlm_extension_base const &ext) override {
        const PacketExtension& from = static_cast<const PacketExtension&>(ext);
        this->packet_id = from.packet_id;
        this->src_stream_id = from.src_stream_id;
        this->priority = from.priority;
        this->inject_time = from.inject_time;
        this->payload_bytes = from.payload_bytes;
    }
};

DECLARE_EXTENDED_PHASE(CREDIT_RETURN);

// Per-stream architectural metrics tracker
class StreamStats {
public:
    static StreamStats& get_instance() {
        static StreamStats instance;
        return instance;
    }

    void record_packet(uint32_t stream_id, double latency_ns, uint64_t bytes) {
        stream_latencies[stream_id].push_back(latency_ns);
        stream_bytes[stream_id] += bytes;
    }

    void reset() {
        stream_latencies.clear();
        stream_bytes.clear();
    }

    double get_avg_latency(uint32_t stream_id) {
        auto& lat = stream_latencies[stream_id];
        if (lat.empty()) return 0.0;
        return std::accumulate(lat.begin(), lat.end(), 0.0) / lat.size();
    }

    uint64_t get_bytes(uint32_t stream_id) {
        return stream_bytes[stream_id];
    }

private:
    std::map<uint32_t, std::vector<double>> stream_latencies;
    std::map<uint32_t, uint64_t> stream_bytes;
};

} // namespace d2d_model
