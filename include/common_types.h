#pragma once
#include <systemc>
#include <tlm>

namespace d2d_model {

struct PacketExtension : public tlm::tlm_extension<PacketExtension> {
    uint64_t packet_id;
    uint32_t src_chiplet_id;
    uint32_t dest_chiplet_id;
    sc_core::sc_time inject_time;
    uint32_t payload_bytes;

    PacketExtension() 
        : packet_id(0), src_chiplet_id(0), dest_chiplet_id(0), 
          inject_time(sc_core::SC_ZERO_TIME), payload_bytes(64) {}

    virtual tlm_extension_base* clone() const override {
        PacketExtension* ext = new PacketExtension();
        ext->packet_id = this->packet_id;
        ext->src_chiplet_id = this->src_chiplet_id;
        ext->dest_chiplet_id = this->dest_chiplet_id;
        ext->inject_time = this->inject_time;
        ext->payload_bytes = this->payload_bytes;
        return ext;
    }

    virtual void copy_from(tlm_extension_base const &ext) override {
        const PacketExtension& from = static_cast<const PacketExtension&>(ext);
        this->packet_id = from.packet_id;
        this->src_chiplet_id = from.src_chiplet_id;
        this->dest_chiplet_id = from.dest_chiplet_id;
        this->inject_time = from.inject_time;
        this->payload_bytes = from.payload_bytes;
    }
};

} // namespace d2d_model
