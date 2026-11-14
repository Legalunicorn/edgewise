import sys
from statistics import median

# usage: python3 benchmarks/gap/summarize_gap.py <gap log>
trials = {}
threads = None
for line in open(sys.argv[1]):
    parts = line.split()
    if line.startswith("threads"):
        threads = int(parts[1])
        trials[threads] = []
    elif line.startswith("Trial Time:"):
        # seconds to milliseconds
        trials[threads].append(float(parts[2]) * 1000)

# drop the first trial of each thread count as warm-up
medians = {t: median(times[1:]) for t, times in trials.items()}
for t, ms in medians.items():
    print(f"{t} threads: {ms:.2f} ms")
best = min(medians, key=medians.get)
print(f"best: {medians[best]:.2f} ms ({best} threads)")
