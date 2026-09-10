#!/bin/bash
cd ~/d2d_tlm_model/build

# Clean previous results
rm -f results.csv
echo "buffer_capacity,injection_interval_ns,num_packets,stall_count,sim_time_ns" > results.csv

echo "Starting Parameter Sweep..."
for buf in 2 8; do
    for interval in 4 6 8 10 12 16 20; do
        echo "Running: Buffer Depth = $buf, Injection Interval = ${interval}ns"
        ./d2d_sim 25 $interval $buf > /dev/null 2>&1
    done
done

echo "Sweep complete! Results saved in ~/d2d_tlm_model/build/results.csv"
