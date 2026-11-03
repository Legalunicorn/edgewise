# Getting started

::: info Under construction
This page is under construction.
:::

## Build and test

Requires a C++20 compiler, CMake 3.20+, a build tool, and Git. GoogleTest downloads during configuration. Run from the repository root in Linux or WSL.

```bash
# configure once per build directory, and again after adding CMake targets
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
# build after source changes
cmake --build build/release
# run correctness tests
ctest --test-dir build/release --output-on-failure
```

## Run BFS

The command-line program runs sequential BFS. Input: `n m`, followed by `m` unweighted, undirected edge pairs using one-based vertex IDs.

```bash
./build/release/edgewise < graph.txt
```

Prints distances from vertex 1, with `-1` for unreachable vertices.

## Benchmarks


Executables: `bfs_bench`, `pagerank_bench`, and `cc_bench`. For example, to benchmark both sequential CC methods, parallel DSU and Afforest across four generated graph sizes:

```bash
# run the benchmark and save its CSV
./build/release/cc_bench > benchmarks/results/cc/new_run.csv
# summarize the timings (Python 3, no extra packages)
python3 benchmarks/summarize.py benchmarks/results/cc/new_run.csv
```

To benchmark one generated graph of a chosen size, pass the number of vertices and edges:

```bash
# one million vertices and 16 million undirected edges
./build/release/cc_bench 1000000 16000000 > benchmarks/results/cc/new_run.csv
```

Large graphs take a while to generate. 16 million vertices and 64 million edges needs several GB of memory.

## Public datasets

`cc_bench` also accepts an edge-list filename. With no argument, it runs the generated graphs above. BFS and PageRank benchmarks currently use generated graphs only.

File format: exactly two nonnegative integer IDs per edge line, with blank lines and `#` comment lines allowed. There is no `n m` header. The loader compresses IDs, removes duplicate and reversed undirected pairs, keeps self-loops once and builds CSR. Only vertices appearing in edges are included. See [graph storage](/graph-storage) for the loader API.

Download the [California road network](https://snap.stanford.edu/data/roadNet-CA.html) and [YouTube network](https://snap.stanford.edu/data/com-Youtube.html) from SNAP. Files in `data/` are ignored by Git.

```bash
# create the data and results directories
mkdir -p data benchmarks/results/cc
# download the California road network
curl -fL https://snap.stanford.edu/data/roadNet-CA.txt.gz -o data/roadNet-CA.txt.gz
# download the YouTube network
curl -fL https://snap.stanford.edu/data/bigdata/communities/com-youtube.ungraph.txt.gz -o data/com-youtube.ungraph.txt.gz
# extract both edge lists and keep the archives
gzip -dk data/roadNet-CA.txt.gz data/com-youtube.ungraph.txt.gz
```

Loading and CSR construction are excluded from timing.

```bash
# benchmark a dataset and save its CSV
./build/release/cc_bench data/roadNet-CA.txt > benchmarks/results/cc/road_ca_new_run.csv
# summarize the timings
python3 benchmarks/summarize.py benchmarks/results/cc/road_ca_new_run.csv
```

For YouTube, use `data/com-youtube.ungraph.txt` and a separate output CSV. File-based results use `seed=-1`. Dataset credits and source papers are listed in the [CC results](/cc/results#dataset-credits).

## ThreadSanitizer

ThreadSanitizer (TSan) reports data races while the tests run. Use a separate build directory, since TSan builds are much slower and are not for benchmarking.

```bash
# let GCC 13's TSan runtime start (lasts until WSL or Linux restarts)
sudo sysctl vm.mmap_rnd_bits=28
# configure an instrumented build with debug information for readable reports
cmake -S . -B build/tsan -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS="-fsanitize=thread"
# build
cmake --build build/tsan
# run the tests under TSan
ctest --test-dir build/tsan --output-on-failure
```

A detected race fails the test and prints both threads' stack traces. CI runs this on every pull request.
