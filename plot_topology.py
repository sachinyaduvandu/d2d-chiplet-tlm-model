import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("build/topology_results.csv")

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 4.5))

destinations = df["destination"]

# Latency plot
ax1.bar(destinations, df["avg_latency_ns"], color=["#2ca02c", "#d62728"], width=0.4)
ax1.set_title("Average E2E Latency by Destination Chiplet")
ax1.set_ylabel("Latency (ns)")
ax1.grid(axis='y', linestyle='--', alpha=0.5)

# Throughput plot
ax2.bar(destinations, df["throughput_gbps"], color=["#2ca02c", "#d62728"], width=0.4)
ax2.set_title("Achieved Bandwidth across Asymmetric D2D Links")
ax2.set_ylabel("Throughput (GB/s)")
ax2.grid(axis='y', linestyle='--', alpha=0.5)

plt.tight_layout()
plt.savefig("topology_analysis.png", dpi=300)
print("Multi-Chiplet topology plot generated: topology_analysis.png")
