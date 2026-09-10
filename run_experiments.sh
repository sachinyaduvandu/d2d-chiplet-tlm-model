#!/bin/bash
cd ~/d2d_tlm_model/build

rm -f arbitration_results.csv
echo "policy,cpu_avg_latency_ns,dma_avg_latency_ns,cpu_bw_gbps,dma_bw_gbps" > arbitration_results.csv

echo "Evaluating Round-Robin Arbitration..."
./d2d_sim RR

echo "Evaluating Strict-Priority Arbitration..."
./d2d_sim PRIO

echo "Done! File content:"
cat arbitration_results.csv
