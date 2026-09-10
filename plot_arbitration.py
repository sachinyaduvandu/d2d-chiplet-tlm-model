import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("build/arbitration_results.csv")

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 4.5))

# 1. Latency Comparison
x = df["policy"]
ax1.bar(x, df["cpu_avg_latency_ns"], width=0.35, label="CPU (Latency-Critical)", color="#1f77b4", align="center")
ax1.bar(x, df["dma_avg_latency_ns"], width=0.35, label="DMA (Bulk)", color="#ff7f0e", align="edge")
ax1.set_title("Average Latency under Cross-Traffic Contention")
ax1.set_ylabel("Latency (ns)")
ax1.legend()
ax1.grid(axis='y', linestyle='--', alpha=0.5)

# 2. Bandwidth Distribution
ax2.bar(x, df["cpu_bw_gbps"], width=0.35, label="CPU Throughput", color="#1f77b4", align="center")
ax2.bar(x, df["dma_bw_gbps"], width=0.35, label="DMA Throughput", color="#ff7f0e", align="edge")
ax2.set_title("Achieved Bandwidth Distribution")
ax2.set_ylabel("Throughput (GB/s)")
ax2.legend()
ax2.grid(axis='y', linestyle='--', alpha=0.5)

plt.tight_layout()
plt.savefig("arbitration_analysis.png", dpi=300)
print("Arbitration analysis plot generated: arbitration_analysis.png")
