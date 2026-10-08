import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import csv
import os

img_dir = "/mnt/d/Documents/SEM V/CN (LAB)/Ex10/images"
os.makedirs(img_dir, exist_ok=True)

# 1. Read Cwnd Data from CSV
cwnd_time = []
cwnd_val = []
try:
    with open('cwnd_TcpNewReno.csv', 'r') as f:
        reader = csv.reader(f)
        next(reader) # skip header
        for row in reader:
            if len(row) >= 2:
                cwnd_time.append(float(row[0]))
                cwnd_val.append(float(row[1]))
except FileNotFoundError:
    print("Error: cwnd_TcpNewReno.csv not found!")

# 2. Read Goodput/Throughput Data from CSV
thr_time = []
thr_val = []
try:
    with open('metrics_TcpNewReno.csv', 'r') as f:
        reader = csv.reader(f)
        next(reader) # skip header
        for row in reader:
            if len(row) >= 2:
                thr_time.append(float(row[0]))
                thr_val.append(float(row[1]))
except FileNotFoundError:
    print("Error: metrics_TcpNewReno.csv not found!")

# 3. Create Plots
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 10))

# Top Graph: Throughput
if thr_time and thr_val:
    ax1.plot(thr_time, thr_val, label='Flow 0 (Throughput)')
ax1.set_title('TCP NewReno Performance')
ax1.set_xlabel('Simulation time (s)')
ax1.set_ylabel('Application throughput (Mbps)')
ax1.grid(True, alpha=0.3)
ax1.legend()

# Bottom Graph: Congestion Window
if cwnd_time and cwnd_val:
    ax2.plot(cwnd_time, cwnd_val, label='Flow 0 (Cwnd)')
ax2.set_xlabel('Simulation time (s)')
ax2.set_ylabel('Congestion window (bytes)')
ax2.grid(True, alpha=0.3)
ax2.legend()

plt.tight_layout()
output_file = f"{img_dir}/Dumbbell_Graph.png"
plt.savefig(output_file)
print(f"Graph generated and saved to: {output_file}")
