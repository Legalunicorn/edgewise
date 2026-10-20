# Edgewise

Shared-memory graph analytics in C++20. Edgewise compares sequential and parallel implementations to investigate synchronization costs, graph storage and worker scaling.

## Implemented

- Sequential BFS as a correctness reference.
- Mutex-based parallel BFS with per-worker output buffers (V1).
- Parallel BFS with atomic vertex discovery and per-worker output buffers (V2).
- Persistent-worker BFS using barriers to coordinate layers (V3).
- GoogleTest correctness tests and a CSV benchmark runner.

PageRank is planned as a second workload.

## Build and test

Requires a C++20 compiler, CMake 3.20+ and Git. GoogleTest is downloaded during configuration.

Run from the repository root in Linux or WSL.

```bash
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
```

Configure a Release build. Usually needed only when first creating the build directory.

```bash
cmake --build build/release
```

Build the application, tests and benchmark. Repeat after editing source files.

```bash
ctest --test-dir build/release --output-on-failure
```

Run the tests and display details for any failures.

## Graph input

The application reads an unweighted, undirected graph from standard input:

```text
n m
u v
...
```

Vertex IDs are one-based. The application runs sequential BFS from vertex 1 and reports distances; unreachable vertices have distance `-1`.

```bash
./build/release/edgewise < graph.txt
```

Read the graph from a file.

## Benchmarks

```bash
./build/release/bfs_bench > bfs_comparison_n200000_m800000_seed67_workers1-16.csv
```

Run sequential BFS and all three parallel versions in one process, saving timings as CSV. Every result is checked against an untimed sequential reference. One warm-up sweep is excluded from the CSV. For each worker count, versions run in the fixed order V1, V2, V3.

The benchmark generates **one fixed graph** with 200,000 vertices, 800,000 undirected edges and seed 67. That same graph is reused for every implementation, worker count, warm-up and recorded repetition. Every traversal starts from internal vertex 0. Five timings are recorded per configuration.

```bash
python3 benchmarks/summarize.py bfs_comparison_n200000_m800000_seed67_workers1-16.csv
```

From the repository root, print a short summary using Python 3; no additional packages are required. The script reports the sequential median and each parallel version's **best observed median**, with its worker count and speedup. It selects the lowest median across tested worker counts, not the fastest individual run.

Sequential median: **36.57 ms** (five samples).

Median traversal times from the combined run:

| Workers | Mutex V1 | Atomic V2 | Persistent V3 |
|---|---:|---:|---:|
| 1 | 56.64 ms | 45.56 ms | 45.00 ms |
| 2 | 81.41 ms | 23.10 ms | 22.67 ms |
| 4 | 100.77 ms | 14.59 ms | 14.83 ms |
| 7 | 120.21 ms | 13.33 ms | 11.67 ms |
| 8 | 120.12 ms | 14.08 ms | 12.09 ms |
| 16 | 142.39 ms | 20.84 ms | 14.45 ms |

Source: [combined benchmark CSV](bfs_comparison_n200000_m800000_seed67_workers1-16.csv).

Timing includes distance initialization, worker startup/shutdown and frontier assembly. Graph generation, result checking and CSV output are excluded. These results cover this single graph, using the fixed execution order described above; other sizes and structures have not yet been measured in this comparison.

## Next steps

- Compare adjacency-list storage with compressed sparse row (CSR) storage.
- Investigate work distribution on graphs with uneven vertex degrees.
- Add sequential and parallel PageRank.
