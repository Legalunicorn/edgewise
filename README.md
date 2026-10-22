# Edgewise

Shared-memory graph analytics in C++20, currently comparing sequential and parallel BFS using CSR graph storage.

Implementations include a mutex baseline (V1), atomic discovery (V2), and persistent workers coordinated with barriers (V3). Correctness tests use GoogleTest.

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
./build/release/bfs_bench > bfs_comparison_csr_four_sizes_seed67.csv
```

Benchmark all implementations across four graph sizes.

```bash
python3 benchmarks/summarize.py bfs_comparison_csr_four_sizes_seed67.csv
```

Print a compact summary per graph. Requires Python 3 with no additional packages.

### Results

Release build on an 8-core / 16-logical-processor laptop. Parallel times below are V3's lowest median across tested worker counts; speedup uses the corresponding sequential median.

| Vertices | Undirected edges | Sequential | V3 | Workers | Speedup |
|---:|---:|---:|---:|---:|---:|
| 200,000 | 800,000 | 9.04 ms | 6.37 ms | 4 | 1.42× |
| 500,000 | 2,000,000 | 42.65 ms | 14.60 ms | 12 | 2.92× |
| 1,000,000 | 4,000,000 | 113.69 ms | 27.94 ms | 14 | 4.07× |
| 1,500,000 | 6,000,000 | 190.47 ms | 40.01 ms | 14 | 4.76× |

- Graphs: binary-tree backbone plus random distinct edges, seed 67, source vertex 0 internally.
- Workers: 1, 2, 3, 4, 6, 8, 10, 12, 14, 16.
- One warm-up sweep, then five measured repetitions; fixed V1 → V2 → V3 order.
- Complete BFS calls are timed, including initialization and worker setup. Graph construction, result checking, and CSV output are excluded.
- Every traversal is checked against the sequential reference.

[Raw results for all implementations](bfs_comparison_csr_four_sizes_seed67.csv).
