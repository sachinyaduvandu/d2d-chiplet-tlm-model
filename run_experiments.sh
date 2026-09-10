#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

cd "${BUILD_DIR}"
rm -f arbitration_results.csv

echo "policy,offered_interval_ns,cpu_lat_ns,dma_lat_ns,overall_avg_lat_ns,p95_lat_ns,throughput_gbps,avg_queue,max_queue,starvations,fairness" > arbitration_results.csv

echo "=========================================================="
echo " Running D2D Chiplet Interconnect Exploration Framework   "
echo "=========================================================="

POLICIES=("FIFO" "RR" "PRIO")
INTERVALS=(2.0 4.0 6.0 8.0 12.0)

for pol in "${POLICIES[@]}"; do
    echo "Simulating Policy: ${pol}"
    for intv in "${INTERVALS[@]}"; do
        # CPU interval = intv, DMA interval = intv / 2 (aggressive bulk)
        dma_intv=$(awk -v i="$intv" 'BEGIN {print i / 2.0}')
        ./d2d_sim "$pol" "$intv" "$dma_intv" > /dev/null 2>&1
    done
done

echo "Simulation sweeps finished successfully! Results stored in build/arbitration_results.csv"
