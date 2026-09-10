#!/bin/bash
cd ~/d2d_tlm_model/build

rm -f results.csv
echo "buffer_capacity,link_bandwidth_gbps,packet_size_bytes,injection_interval_ns,offered_load,achieved_bw_gbps,avg_latency_ns,p95_latency_ns,stall_count,link_util_pct" > results.csv

echo "Starting Architectural Design-Space Exploration..."

# Experiment: Sweep Offered Load across Bandwidth (16 GB/s vs 64 GB/s) with 128B Packets
for bw in 16.0 64.0; do
    for interval in 2.0 3.0 4.0 6.0 8.0 12.0 16.0 20.0; do
        echo "Running: BW = ${bw} GB/s, Interval = ${interval} ns, Buffer = 8"
        ./d2d_sim 50 $interval 8 $bw 128 > /dev/null 2>&1
    done
done

echo "Exploration Complete! Results saved to build/results.csv"
