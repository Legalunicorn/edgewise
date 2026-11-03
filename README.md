# Edgewise

[![CI](https://github.com/Legalunicorn/edgewise/actions/workflows/ci.yml/badge.svg)](https://github.com/Legalunicorn/edgewise/actions/workflows/ci.yml)

Shared-memory graph analytics in C++20, comparing sequential and parallel BFS, connected components and PageRank over CSR graph storage.


- **BFS:** mutex discovery (V1), atomic discovery (V2), persistent workers (V3), and parallel frontier assembly (V4).
- **Connected components:** sequential BFS and DSU baselines, plus parallel CC using concurrent DSU and Afforest, both with dynamic load balancing.
- **PageRank:** sequential and parallel pull implementations with dangling-score redistribution.

<p align="center">
  <img src="docs/public/animations/parallel-bfs-radial.svg" width="520" alt="Animation of parallel BFS with four workers expanding ring by ring from a source vertex">
</p>

> Note: generative AI was used to create the visualizations for this project.

Parallel BFS (V3) on a generated graph: each colour is a worker scanning its slice of the frontier, a red ring is a lost compare-and-swap, and grey vertices are finished. Workers meet at a barrier after every layer.


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
| [CC using DSU](benchmarks/results/cc/cc_chunked_generated.csv) | 73.80 | 18.51 | 16 | 3.99× |
| [CC using Afforest](benchmarks/results/cc/cc_chunked_generated.csv) | 73.80 | 16.59 | 14 | 4.45× |
| [PageRank](benchmarks/results/pagerank/pagerank_all.csv) | 1,885.92 | 429.22 | 16 | 4.39× |

Times are medians of five measured runs after warm-up, using the worker count with the lowest median. CC is compared with the faster sequential baseline, DSU.

Afforest is slightly slower than parallel DSU on small, low-degree graphs, but is **1.94× faster at average degree 32** and **1.62× faster on a 16-million-vertex graph**. See the [CC results](https://legalunicorn.github.io/edgewise/cc/results) for all graphs, including the California roads and YouTube datasets, and raw measurements.
