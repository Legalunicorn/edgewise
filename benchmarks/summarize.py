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

    baselines = {}
    for implementation, label in (
        ("sequential", "Sequential"),
        ("cc_sequential_bfs", "Sequential BFS"),
        ("cc_sequential_dsu", "Sequential DSU"),
    ):
        if (implementation, 1) in samples:
            baselines[label] = median(samples[(implementation, 1)])
            print(f"{label}: {baselines[label]:.2f} ms")

    if not baselines:
        raise SystemExit("No sequential baseline found for this graph.")

    for implementation, label in (
        ("mutex_v1", "V1 mutex"),
        ("atomic_v2", "V2 atomic"),
        ("persistent_v3", "V3 persistent"),
        ("assembly_v4", "V4 parallel assembly"),
        ("pagerank_parallel", "Parallel PageRank"),
        ("cc_parallel_dsu", "Parallel CC DSU"),
    ):
        configurations = [
            (median(times), workers)
            for (name, workers), times in samples.items()
            if name == implementation
        ]
        if not configurations:  # Older CSVs may not contain every implementation.
            continue
        elapsed, workers = min(configurations)  # Lowest median, not fastest individual run.
        speedups = ", ".join(
            f"{baseline / elapsed:.2f}x speedup"
            + (f" vs {name}" if len(baselines) > 1 else "")
            for name, baseline in baselines.items()
        )
        print(f"{label}: {elapsed:.2f} ms, workers={workers}, {speedups}")
