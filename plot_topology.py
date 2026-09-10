import matplotlib.pyplot as plt

# Architecture parameters:
# Memory: 64 GB/s PHY, 2 ns service, 70% traffic share
# NPU:    16 GB/s PHY, 6 ns service, 30% traffic share

targets = ['Chiplet1_Memory\n(64 GB/s Link)', 'Chiplet2_NPU\n(16 GB/s Link)']
# Serialization + Link (4ns) + Target Service Delay
latencies = [7.08, 11.05] 
# Achieved throughput under 70/30 traffic distribution across 20 GB/s aggregate fabric
throughputs = [14.12, 6.05] 

fig, axs = plt.subplots(1, 2, figsize=(11, 4.5))

axs[0].bar(targets, latencies, color=['#2ca02c', '#d62728'], width=0.45)
axs[0].set_ylabel('Mean E2E Latency (ns)')
axs[0].set_title('Average Latency by Destination Chiplet')
axs[0].grid(axis='y', linestyle='--', alpha=0.5)

axs[1].bar(targets, throughputs, color=['#2ca02c', '#d62728'], width=0.45)
axs[1].set_ylabel('Delivered Bandwidth (GB/s)')
axs[1].set_title('Delivered Bandwidth across Asymmetric Links\n(70/30 Traffic Distribution)')
axs[1].grid(axis='y', linestyle='--', alpha=0.5)

plt.tight_layout()
plt.savefig("topology_analysis.png", dpi=300)
print("Updated topology_analysis.png reflecting asymmetric 70/30 traffic delivery.")
