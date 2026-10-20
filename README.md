# Edgewise

Shared-memory graph analytics in C++20. Edgewise compares sequential and parallel implementations to investigate synchronization costs, graph storage and worker scaling.

## Implemented

- Sequential BFS as a correctness reference.
- Parallel BFS with atomic vertex discovery and per-worker output buffers.
- Persistent-worker BFS using barriers to coordinate layers.
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
./build/release/bfs_bench > bfs_persistent_v3_results.csv
```

Run the currently selected BFS implementation and save timings as CSV. Each parallel result is checked against sequential BFS.

The current workload uses 200,000 vertices, 800,000 undirected edges and seed 67, with five repetitions for each worker count.

Preliminary median traversal times:

| Workers | Atomic BFS (V2) | Persistent workers (V3) |
|---|---:|---:|
| 1 | 45.18 ms | 49.68 ms |
| 4 | 15.41 ms | 15.18 ms |
| 7 | 13.60 ms | 11.49 ms |
| 16 | 17.67 ms | 14.06 ms |

These measurements were collected in separate Release-build sessions on AC power. They cover one generated graph family and are not general performance guarantees.

## Next steps

- Compare adjacency-list storage with compressed sparse row (CSR) storage.
- Investigate work distribution on graphs with uneven vertex degrees.
- Add sequential and parallel PageRank.
