# Edgewise

Shared-memory graph analytics in C++20. Edgewise compares sequential and parallel implementations to investigate synchronization costs, graph storage and worker scaling.

## Implemented

- Sequential BFS as a correctness reference.
- Mutex-based parallel BFS with per-worker output buffers (V1).
- Parallel BFS with atomic vertex discovery and per-worker output buffers (V2).
- Persistent-worker BFS using barriers to coordinate layers (V3).
- CSR storage shared by all BFS implementations.
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
./build/release/bfs_bench > bfs_comparison_csr_four_sizes_seed67.csv
```

Run sequential BFS and V1-V3 on four generated CSR graphs, saving timings as CSV:

| Vertices | Undirected edges | Seed |
|---:|---:|---:|
| 200,000 | 800,000 | 67 |
| 500,000 | 2,000,000 | 67 |
| 1,000,000 | 4,000,000 | 67 |
| 1,500,000 | 6,000,000 | 67 |

Each graph starts with an undirected binary tree, then adds random distinct edges. For each size, the same graph is reused by every implementation and repetition, with internal vertex 0 as the source. The parallel worker counts are **1, 2, 3, 4, 6, 8, 10, 12, 14, 16**.

One warm-up sweep per graph is excluded from the CSV; five timings are recorded per configuration. Versions run in the fixed order V1, V2, V3 for each worker count. Every traversal is checked against an untimed sequential reference; a mismatch stops the program with a nonzero exit code.

The CSV records `implementation,vertices,edges,seed,workers,run,time_ms`. Timing includes distance initialization, worker startup/shutdown, frontier assembly and conversion to the returned distance vector. Graph generation, CSR construction, result checking and CSV output are excluded.

```bash
python3 benchmarks/summarize.py bfs_comparison_csr_four_sizes_seed67.csv
```

Print one compact summary per graph using Python 3; no additional packages are required. Each summary uses that graph's sequential median and each parallel version's **best observed median** across tested worker counts. Older single-graph CSVs are also supported.

### Current CSR results

Best observed persistent-worker (V3) medians from the four-size run:

| Vertices | Undirected edges | Sequential median | V3 median | Workers | Speedup |
|---:|---:|---:|---:|---:|---:|
| 200,000 | 800,000 | 9.04 ms | 6.37 ms | 4 | 1.42x |
| 500,000 | 2,000,000 | 42.65 ms | 14.60 ms | 12 | 2.92x |
| 1,000,000 | 4,000,000 | 113.69 ms | 27.94 ms | 14 | 4.07x |
| 1,500,000 | 6,000,000 | 190.47 ms | 40.01 ms | 14 | 4.76x |

Source: [four-size CSR benchmark CSV](bfs_comparison_csr_four_sizes_seed67.csv). Measurements were taken on a laptop with 8 physical cores and 16 logical processors using a Release build. Speedups use each graph's own sequential median. These are observations for one seed and one graph family; the fixed execution order and laptop power/thermal conditions can affect timings.

V3 also uses a read-before-CAS discovery check and relaxed distance initialization. Its comparison with V2 therefore includes these optimizations as well as persistent workers.

### Historical results before CSR

These measurements predate both CSR and the sequential visited-state refactor. They are retained as historical results, not a controlled measurement of CSR's effect. Current measurements are reported above.

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

This historical run used one graph with 200,000 vertices, 800,000 undirected edges and seed 67, and tested every worker count from 1 to 16. It does not represent the current four-size CSR benchmark.

## Next steps

- Measure CSR against the immediate pre-CSR commit under matching conditions.
- Investigate work distribution on graphs with uneven vertex degrees.
- Add sequential and parallel PageRank.
