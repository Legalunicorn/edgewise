import csv
import sys
from collections import defaultdict
from statistics import median


# this file is to generate simple statistics from the raw benchmark csv

if len(sys.argv) != 2:
    raise SystemExit("Usage: python3 benchmarks/summarize.py <results.csv>")

samples = defaultdict(list)  # Collect timings for each configuration.

with open(sys.argv[1], newline="", encoding="utf-8-sig") as file:
    for row in csv.DictReader(file):
        key = (row["implementation"], int(row["workers"]))
        samples[key].append(float(row["time_ms"]))

baseline = median(samples[("sequential", 1)])  # Sequential reference time.

print(f"Sequential: {baseline:.2f} ms")

# Show the fastest median across tested worker counts, not the fastest single run.
for implementation, label in (
    ("mutex_v1", "V1 mutex"),
    ("atomic_v2", "V2 atomic"),
    ("persistent_v3", "V3 persistent"),
):
    configurations = [
        (median(times), workers)
        for (name, workers), times in samples.items()
        if name == implementation
    ]
    elapsed, workers = min(configurations)  # Choose the lowest median time.
    print(f"{label}: {elapsed:.2f} ms, workers={workers}, {baseline / elapsed:.2f}x speedup")
