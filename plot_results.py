import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("build/results.csv")

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

# 1. Throughput Saturation Curve
for bw, group in df.groupby("link_bandwidth_gbps"):
    sorted_group = group.sort_values("offered_load")
    ax1.plot(sorted_group["offered_load"], sorted_group["achieved_bw_gbps"], marker='o', label=f"D2D Link: {bw} GB/s")

ax1.set_title("Offered Load vs. Achieved Throughput (Saturation)")
ax1.set_xlabel("Offered Load (Packets / ns)")
ax1.set_ylabel("Achieved Throughput (GB/s)")
ax1.grid(True, linestyle="--", alpha=0.5)
ax1.legend()

# 2. Latency vs. Offered Load (Average and p95 tail)
for bw, group in df.groupby("link_bandwidth_gbps"):
    sorted_group = group.sort_values("offered_load")
    ax2.plot(sorted_group["offered_load"], sorted_group["avg_latency_ns"], marker='s', label=f"Avg Latency ({bw} GB/s)")
    ax2.plot(sorted_group["offered_load"], sorted_group["p95_latency_ns"], linestyle=":", label=f"p95 Latency ({bw} GB/s)")

ax2.set_title("Latency vs. Offered Load Curve")
ax2.set_xlabel("Offered Load (Packets / ns)")
ax2.set_ylabel("End-to-End Latency (ns)")
ax2.grid(True, linestyle="--", alpha=0.5)
ax2.legend()

plt.tight_layout()
plt.savefig("d2d_performance_curve.png", dpi=300)
print("Updated publication-grade plot: d2d_performance_curve.png")
