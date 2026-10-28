import csv
import sys
from collections import defaultdict
from statistics import median


if len(sys.argv) != 2:
    raise SystemExit("Usage: python3 benchmarks/summarize.py <results.csv>")

graphs = defaultdict(lambda: defaultdict(list))  # Keep graph sizes and seeds separate.

with open(sys.argv[1], newline="", encoding="utf-8-sig") as file:
    for row in csv.DictReader(file):
        graph = (
            (int(row["vertices"]), int(row["edges"]), int(row["seed"]))
            if "vertices" in row else None  # Also support the older single-graph CSVs.
        )
        key = (row["implementation"], int(row["workers"]))
        graphs[graph][key].append(float(row["time_ms"]))

if not graphs:
    raise SystemExit("No benchmark measurements found in the CSV.")

for index, (graph, samples) in enumerate(graphs.items()):
    if index:
        print()  # Separate the compact summaries with one blank line.
    if graph is not None:
        n, m, seed = graph
        print(f"Graph: {n:,} vertices, {m:,} edges, seed={seed}")

    baseline = median(samples[("sequential", 1)])  # Use this graph's own baseline.
    print(f"Sequential: {baseline:.2f} ms")

    for implementation, label in (
        ("mutex_v1", "V1 mutex"),
        ("atomic_v2", "V2 atomic"),
        ("persistent_v3", "V3 persistent"),
        ("assembly_v4", "V4 parallel assembly"),
        ("pagerank_parallel", "Parallel PageRank"),
    ):
        configurations = [
            (median(times), workers)
            for (name, workers), times in samples.items()
            if name == implementation
        ]
        if not configurations:  # Older CSVs may not contain every implementation.
            continue
        elapsed, workers = min(configurations)  # Lowest median, not fastest individual run.
        print(f"{label}: {elapsed:.2f} ms, workers={workers}, {baseline / elapsed:.2f}x speedup")
