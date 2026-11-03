# CC benchmark results

This page compares [parallel CC using concurrent DSU](/cc/parallel-dsu) with [Afforest](/cc/afforest). Both use dynamic chunks to balance work across workers. 

[Sequential CC using DSU](/cc/sequential-dsu) is the baseline, since it was faster than [sequential CC using BFS](/cc/sequential-bfs) on every graph. 


## Setup

- Release build. Times are median milliseconds over five measured runs after one warm-up run.
- Each parallel result uses the worker count with the lowest median among `1, 2, 3, 4, 6, 8, 10, 12, 14, 16`. The worker count is shown in brackets.
- Timing covers the whole CC call, including DSU initialization, worker startup and label collection. Loading, CSR construction and correctness checks are excluded.
- Every result is checked against sequential CC using BFS outside timing.
- Generated graphs are random undirected graphs with seed `67`.

## Results

**Average degree** is `2E / V`, the average number of neighbours per vertex. **Afforest vs DSU** is the parallel DSU time divided by the Afforest time, so values above `1` mean Afforest is faster.

| Graph | Vertices (M) | Edges (M) | Average degree | Sequential DSU | Parallel DSU | Afforest | Afforest vs DSU |
|---|---:|---:|---:|---:|---:|---:|---:|
| Generated | 0.2 | 0.8 | 8 | 6.51 | 3.31 (10) | 3.90 (8) | 0.85× |
| Generated | 0.5 | 2.0 | 8 | 18.98 | 6.29 (14) | 6.96 (14) | 0.90× |
| Generated | 1.0 | 4.0 | 8 | 54.07 | 12.21 (14) | 11.74 (16) | 1.04× |
| Generated | 1.5 | 6.0 | 8 | 73.80 | 18.51 (16) | 16.59 (14) | 1.12× |
| [California roads](https://snap.stanford.edu/data/roadNet-CA.html) | 2.0 | 2.8 | 2.8 | 41.35 | 11.63 (14) | 14.00 (14) | 0.83× |
| [YouTube](https://snap.stanford.edu/data/com-Youtube.html) | 1.1 | 3.0 | 5.3 | 30.98 | 9.14 (16) | 8.80 (10) | 1.04× |
| Generated, high degree | 1.0 | 16.0 | 32 | 98.19 | 28.72 (16) | 14.80 (14) | **1.94×** |
| Generated, large | 16.0 | 64.0 | 8 | 2,688.94 | 555.88 (16) | 343.24 (16) | **1.62×** |

Sequential timings varied by up to about 25% between benchmark sessions, while parallel timings stayed close. Compare the parallel columns within a row rather than speedups across rows.

## Speedup over sequential DSU

The same measurements as speedups over the sequential DSU baseline: sequential time divided by parallel time, so higher is faster.

| Graph | Vertices (M) | Edges (M) | Parallel DSU | Afforest |
|---|---:|---:|---:|---:|
| Generated | 0.2 | 0.8 | 1.97× | 1.67× |
| Generated | 0.5 | 2.0 | 3.02× | 2.73× |
| Generated | 1.0 | 4.0 | 4.43× | 4.61× |
| Generated | 1.5 | 6.0 | 3.99× | 4.45× |
| California roads | 2.0 | 2.8 | 3.56× | 2.95× |
| YouTube | 1.1 | 3.0 | 3.39× | 3.52× |
| Generated, high degree | 1.0 | 16.0 | 3.42× | **6.63×** |
| Generated, large | 16.0 | 64.0 | 4.84× | **7.83×** |

These ratios depend on the sequential time, which varied more between sessions than the parallel times. Treat differences of a few tenths across rows as noise.

## Findings

1. **Afforest is slightly slower than parallel DSU on small or low-degree graphs**
     - It's slower on California roads, where the two neighbour rounds already cover most edges and the extra passes cost time.
2.  **Afforest beats DSU as the average degree grows.** 
    - At degree 32,  it's 1.94× faster than parallel DSU, which is itself 3.42× faster than sequential DSU. 
    - **Afforest also pulls ahead once the graph is larger than the cache.** With 16 million vertices, the parent array takes 64 MB and no longer fits in my device's 16 MB L3 cache. Every edge that wasnt skipped with DSU slowed algorithm.
3. **Dynamic chunks matter for skewed graphs.** 
    - Before chunking, parallel DSU took 13.20 ms on YouTube, with chunks it cut down to 9.14 ms.

## Raw measurements

- [Generated graphs](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_chunked_generated.csv), [high degree](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_degree32.csv), [large](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_16m.csv)
- [California roads](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_chunked_road_ca.csv), [YouTube](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_chunked_youtube.csv)
- Before parallel DSU used chunks: [generated](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_afforest_generated.csv), [California roads](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_afforest_road_ca.csv), [YouTube](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_afforest_youtube.csv)

Older measurements from before Afforest are in the [results folder](https://github.com/Legalunicorn/edgewise/tree/main/benchmarks/results/cc).

### Dataset credits

Both datasets are distributed by [SNAP](https://snap.stanford.edu/data/). The dataset pages list these source papers:

- California roads: J. Leskovec, K. Lang, A. Dasgupta and M. Mahoney, [Community Structure in Large Networks: Natural Cluster Sizes and the Absence of Large Well-Defined Clusters](https://arxiv.org/abs/0810.1355), Internet Mathematics, 2009.
- YouTube: J. Yang and J. Leskovec, [Defining and Evaluating Network Communities based on Ground-truth](https://arxiv.org/abs/1205.6233), ICDM, 2012.

The YouTube file contains the largest connected component of the network. These benchmarks use its edges, not the separate community labels.

See [Getting started](/getting-started#public-datasets) for download and run commands.
