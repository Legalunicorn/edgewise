# Edgewise

[![CI](https://github.com/Legalunicorn/edgewise/actions/workflows/ci.yml/badge.svg)](https://github.com/Legalunicorn/edgewise/actions/workflows/ci.yml)

Shared-memory graph analytics in C++20, comparing sequential and parallel BFS, PageRank, and connected components over CSR graph storage.

- **BFS:** mutex discovery (V1), atomic discovery (V2), persistent workers (V3), and parallel frontier assembly (V4).
- **PageRank:** sequential and parallel pull implementations with dangling-score redistribution.
- **Connected components:** sequential BFS and DSU baselines, plus parallel CC using concurrent DSU. Afforest is planned.

Correctness tests use GoogleTest, and CI also runs them under ThreadSanitizer. Benchmarks check results against sequential references outside timing.

## [Documentation](https://legalunicorn.github.io/edgewise/)

## Build and test

Requires a C++20 compiler, CMake 3.20+ and Git. Run from the repository root in Linux or WSL.

```bash
# configure
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
# build
cmake --build build/release
# run tests
ctest --test-dir build/release --output-on-failure
```

See [Getting started](https://legalunicorn.github.io/edgewise/getting-started) for the ThreadSanitizer build, the BFS program, benchmarks and public datasets.

## Performance

Saved Release-build results on an 8-core / 16-logical-processor laptop, using generated graphs with 1.5 million vertices and 6 million undirected edges:

| Implementation | Sequential (ms) | Parallel (ms) | Workers | Speedup |
|---|---:|---:|---:|---:|
| [BFS V3](benchmarks/results/bfs/bfs_comparison_v1-v4_last.csv) | 287.47 | 69.71 | 14 | 4.12× |
| [PageRank](benchmarks/results/pagerank/pagerank_all.csv) | 1,885.92 | 429.22 | 16 | 4.39× |
| [CC using DSU](benchmarks/results/cc/cc_all.csv) | 88.56 | 18.83 | 14 | 4.70× |

Times are medians of five measured runs after warm-up, using the worker count with the lowest median. CC is compared with the faster sequential baseline, DSU. The generated CC measurements predate parallel label collection and have not been rerun.

On public datasets, parallel CC using DSU took **12.75 ms on California roads (3.40× over sequential DSU)** and **13.37 ms on YouTube (2.06×)**. See the [CC results](https://legalunicorn.github.io/edgewise/cc/results) for dataset credits and raw measurements.
