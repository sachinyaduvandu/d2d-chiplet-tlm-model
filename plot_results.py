import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("build/results.csv")

plt.figure(figsize=(8, 5))
for buf, group in df.groupby("buffer_capacity"):
    plt.plot(group["injection_interval_ns"], group["stall_count"], marker='o', label=f"Buffer Depth = {buf}")

plt.title("Inter-Chiplet D2D Bridge Stall Analysis")
plt.xlabel("Packet Injection Interval (ns) [Lower = Higher Congestion]")
plt.ylabel("Total Buffer Stall Cycles")
plt.grid(True, linestyle="--", alpha=0.6)
plt.legend()
plt.tight_layout()
plt.savefig("d2d_performance_curve.png")
print("Performance plot generated: d2d_performance_curve.png")
