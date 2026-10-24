# Edgewise

Shared-memory graph analytics in C++20, currently comparing sequential and parallel BFS over CSR graph storage.

BFS implementations cover mutex discovery (V1), atomic discovery (V2), persistent workers (V3), and parallel frontier assembly (V4). Correctness tests use GoogleTest.

[Documentation](https://legalunicorn.github.io/edgewise/) · [Parallel BFS](https://legalunicorn.github.io/edgewise/bfs/implementations)

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

Input: `n m`, followed by `m` unweighted, undirected edge pairs using one-based vertex IDs.

```bash
./build/release/edgewise < graph.txt
```

Prints distances from vertex 1, with `-1` for unreachable vertices.

## Performance

V3 achieved **4.12× speedup** on a generated graph with 1.5 million vertices and 6 million undirected edges, using 14 workers on an 8-core / 16-logical-processor laptop. The Release-build medians were **287.47 ms sequential** and **69.71 ms parallel**, over five measured repetitions; 14 workers gave the lowest median among tested counts for that graph.

See the [documentation](https://legalunicorn.github.io/edgewise/) for the comparison and [raw results](benchmarks/results/bfs/bfs_comparison_v1-v4_last.csv) for all implementations.

```bash
./build/release/bfs_bench > benchmarks/results/bfs/new_run.csv
```

Benchmark all implementations across four graph sizes.

```bash
python3 benchmarks/summarize.py benchmarks/results/bfs/new_run.csv
```

Summarize the results. Requires Python 3 with no additional packages.
