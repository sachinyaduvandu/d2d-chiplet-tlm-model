import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("build/arbitration_results.csv")

fig, axs = plt.subplots(2, 2, figsize=(14, 10))

policies = df["policy"].unique()
colors = {"FIFO": "#d62728", "RR": "#1f77b4", "PRIO": "#2ca02c"}

# 1. CPU Latency vs Load
for pol in policies:
    sub = df[df["policy"] == pol].sort_values("offered_interval_ns")
    axs[0, 0].plot(1.0 / sub["offered_interval_ns"], sub["cpu_lat_ns"], marker='o', label=pol, color=colors[pol])
axs[0, 0].set_title("CPU Latency vs. Offered Load")
axs[0, 0].set_xlabel("Offered Load (Packets / ns)")
axs[0, 0].set_ylabel("Latency (ns)")
axs[0, 0].grid(True, linestyle="--", alpha=0.5)
axs[0, 0].legend()

# 2. Total Achieved Throughput
for pol in policies:
    sub = df[df["policy"] == pol].sort_values("offered_interval_ns")
    axs[0, 1].plot(1.0 / sub["offered_interval_ns"], sub["throughput_gbps"], marker='s', label=pol, color=colors[pol])
axs[0, 1].set_title("Throughput vs. Offered Load (HOL Impact)")
axs[0, 1].set_xlabel("Offered Load (Packets / ns)")
axs[0, 1].set_ylabel("Throughput (GB/s)")
axs[0, 1].grid(True, linestyle="--", alpha=0.5)
axs[0, 1].legend()

# 3. P95 Tail Latency
for pol in policies:
    sub = df[df["policy"] == pol].sort_values("offered_interval_ns")
    axs[1, 0].plot(1.0 / sub["offered_interval_ns"], sub["p95_lat_ns"], marker='^', label=pol, color=colors[pol])
axs[1, 0].set_title("System-Wide P95 Tail Latency")
axs[1, 0].set_xlabel("Offered Load (Packets / ns)")
axs[1, 0].set_ylabel("P95 Latency (ns)")
axs[1, 0].grid(True, linestyle="--", alpha=0.5)
axs[1, 0].legend()

# 4. Jain's Fairness Index
for pol in policies:
    sub = df[df["policy"] == pol].sort_values("offered_interval_ns")
    axs[1, 1].plot(1.0 / sub["offered_interval_ns"], sub["fairness"], marker='d', label=pol, color=colors[pol])
axs[1, 1].set_title("Jain's Fairness Index across Streams")
axs[1, 1].set_xlabel("Offered Load (Packets / ns)")
axs[1, 1].set_ylabel("Fairness Index [0 - 1.0]")
axs[1, 1].grid(True, linestyle="--", alpha=0.5)
axs[1, 1].legend()

plt.tight_layout()
plt.savefig("arbitration_analysis.png", dpi=300)
print("Updated publication plot: arbitration_analysis.png")
