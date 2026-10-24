# Edgewise

Shared-memory graph analytics in C++20, currently comparing sequential and parallel BFS using CSR graph storage.

Implementations include a mutex baseline (V1), atomic discovery (V2), persistent workers (V3), and parallel frontier assembly (V4). Correctness tests use GoogleTest.

## Build and test

Requires a C++20 compiler, CMake 3.20+, a build tool, and Git. GoogleTest downloads during configuration. Run from the repository root in Linux or WSL.

```bash
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
```

Configure once per build directory.

```bash
cmake --build build/release
```

Build after source changes.

```bash
ctest --test-dir build/release --output-on-failure
```

Run correctness tests.

## Run

Input is an unweighted, undirected graph: `n m`, followed by `m` edge pairs using one-based vertex IDs.

```bash
./build/release/edgewise < graph.txt
```

Print BFS distances from vertex 1; unreachable vertices have distance `-1`.

## Benchmarks

```bash
./build/release/bfs_bench > bfs_comparison.csv
```

Benchmark all implementations across four graph sizes.

```bash
python3 benchmarks/summarize.py bfs_comparison.csv
```

Print a compact summary per graph. Requires Python 3 with no additional packages.

### Results

Release build on an 8-core / 16-logical-processor laptop. Each parallel time is the lowest median across tested worker counts; parentheses show the selected worker count. Speedups use the sequential median from the same run.

| Vertices | Undirected edges | Sequential | V3 (workers) | V4 (workers) | V3 speedup | V4 speedup |
|---:|---:|---:|---:|---:|---:|---:|
| 200,000 | 800,000 | 17.02 ms | 12.30 ms (8) | 14.23 ms (4) | 1.38× | 1.20× |
| 500,000 | 2,000,000 | 72.15 ms | 25.92 ms (10) | 28.80 ms (8) | 2.78× | 2.51× |
| 1,000,000 | 4,000,000 | 178.31 ms | 47.63 ms (16) | 53.71 ms (14) | 3.74× | 3.32× |
| 1,500,000 | 6,000,000 | 287.47 ms | 69.71 ms (14) | 73.64 ms (14) | 4.12× | 3.90× |

V3 remains the preferred implementation: V4's parallel frontier assembly was slower at all four sizes in this run.

- Graphs: binary-tree backbone plus random distinct edges, seed 67, source vertex 0 internally.
- Workers: 1, 2, 3, 4, 6, 8, 10, 12, 14, 16.
- One warm-up sweep, then five measured repetitions; fixed V1 → V2 → V3 → V4 order per worker count.
- Complete BFS calls are timed, including initialization and worker setup. Graph construction, result checking, and CSV output are excluded.
- Every traversal is checked against the sequential reference.

[Raw results for all implementations](bfs_comparison_v1-v4_last.csv).
