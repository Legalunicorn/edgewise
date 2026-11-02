# Edgewise

Shared-memory graph analytics in C++20, comparing sequential and parallel BFS, PageRank, and connected components over CSR graph storage.

- **BFS:** mutex discovery (V1), atomic discovery (V2), persistent workers (V3), and parallel frontier assembly (V4).
- **PageRank:** sequential and parallel pull implementations with dangling-score redistribution.
- **Connected components:** sequential BFS and DSU baselines, plus parallel CC using concurrent DSU. Afforest is planned.

Correctness tests use GoogleTest. Benchmarks check results against sequential references outside timing.

[Documentation](https://legalunicorn.github.io/edgewise/)

## Build and test

Requires a C++20 compiler, CMake 3.20+, a build tool, and Git. GoogleTest downloads during configuration. Run from the repository root in Linux or WSL.

```bash
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
```

Configure once per build directory; rerun after adding CMake targets.

```bash
cmake --build build/release
```

Build after source changes.

```bash
ctest --test-dir build/release --output-on-failure
```

Run correctness tests.

## Run BFS

The command-line program runs sequential BFS. Input: `n m`, followed by `m` unweighted, undirected edge pairs using one-based vertex IDs.

```bash
./build/release/edgewise < graph.txt
```

Prints distances from vertex 1, with `-1` for unreachable vertices.

## Performance

Saved Release-build results on an 8-core / 16-logical-processor laptop, using generated graphs with 1.5 million vertices and 6 million undirected edges:

| Implementation | Sequential (ms) | Parallel (ms) | Workers | Speedup |
|---|---:|---:|---:|---:|
| [BFS V3](benchmarks/results/bfs/bfs_comparison_v1-v4_last.csv) | 287.47 | 69.71 | 14 | 4.12× |
| [PageRank](benchmarks/results/pagerank/pagerank_all.csv) | 1,885.92 | 429.22 | 16 | 4.39× |
| [CC using DSU](benchmarks/results/cc/cc_all.csv) | 88.56 | 18.83 | 14 | 4.70× |

Times are medians of five measured runs after warm-up. Each parallel result uses the worker count with the lowest median. CC uses the faster sequential baseline, DSU; its results are preliminary pending Afforest. The generated CC measurements predate parallel label collection and have not been rerun with the current implementation.

BFS and PageRank use a binary-tree backbone plus random edges; CC uses random edges without enforcing connectivity. These measurements cover generated inputs on one machine.

On public datasets, parallel CC using DSU took **12.75 ms on California roads (3.40× over sequential DSU)** and **13.37 ms on YouTube (2.06×)** with parallel label collection. See the [CC results](https://legalunicorn.github.io/edgewise/cc/results) for dataset credits, timings and raw measurements.

## Benchmarks

Executables: `bfs_bench`, `pagerank_bench`, and `cc_bench`. For example:

```bash
./build/release/cc_bench > benchmarks/results/cc/new_run.csv
```

Benchmark both sequential CC methods and parallel DSU across four graph sizes.

```bash
python3 benchmarks/summarize.py benchmarks/results/cc/new_run.csv
```

Summarize the results. Requires Python 3 with no additional packages.

### Public datasets

`cc_bench` also accepts an edge-list filename. With no argument, it still runs the generated graphs above. BFS and PageRank benchmarks currently use generated graphs only.

File format: exactly two nonnegative integer IDs per edge line, with blank lines and `#` comment lines allowed. There is no `n m` header. The loader compresses IDs, removes duplicate and reversed undirected pairs, keeps self-loops once and builds CSR. Only vertices appearing in edges are included. See [graph storage](https://legalunicorn.github.io/edgewise/graph-storage) for the loader API.

Download the [California road network](https://snap.stanford.edu/data/roadNet-CA.html) and [YouTube network](https://snap.stanford.edu/data/com-Youtube.html) from SNAP:

```bash
mkdir -p data benchmarks/results/cc
```

Create the data and results directories. Downloaded files in `data/` are ignored by Git.

```bash
curl -fL https://snap.stanford.edu/data/roadNet-CA.txt.gz -o data/roadNet-CA.txt.gz
```

Download the California road network.

```bash
gzip -dk data/roadNet-CA.txt.gz
```

Extract the edge list and keep the archive.

```bash
curl -fL https://snap.stanford.edu/data/bigdata/communities/com-youtube.ungraph.txt.gz -o data/com-youtube.ungraph.txt.gz
```

Download the YouTube network.

```bash
gzip -dk data/com-youtube.ungraph.txt.gz
```

Extract the YouTube edge list.

```bash
./build/release/cc_bench data/roadNet-CA.txt > benchmarks/results/cc/road_ca_new_run.csv
```

Benchmark both sequential CC methods and parallel DSU. Loading and CSR construction are excluded from timing.

```bash
python3 benchmarks/summarize.py benchmarks/results/cc/road_ca_new_run.csv
```

Summarize the saved timings. To benchmark YouTube, use `data/com-youtube.ungraph.txt` and a separate output CSV. File-based results use `seed=-1`.

Dataset credits and source papers are listed in the [CC results](https://legalunicorn.github.io/edgewise/cc/results#dataset-credits).
