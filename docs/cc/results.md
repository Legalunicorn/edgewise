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
| Generated | 0.2 | 0.8 | 8 | 6.83 | 3.07 (10) | 3.82 (10) | 0.80× |
| Generated | 0.5 | 2.0 | 8 | 20.11 | 5.81 (14) | 7.05 (14) | 0.82× |
| Generated | 1.0 | 4.0 | 8 | 76.61 | 10.94 (16) | 11.91 (14) | 0.92× |
| Generated | 1.5 | 6.0 | 8 | 148.09 | 16.17 (16) | 17.21 (14) | 0.94× |
| [California roads](https://snap.stanford.edu/data/roadNet-CA.html) | 2.0 | 2.8 | 2.8 | 42.75 | 9.70 (16) | 10.36 (14) | 0.94× |
| [YouTube](https://snap.stanford.edu/data/com-Youtube.html) | 1.1 | 3.0 | 5.3 | 30.52 | 6.91 (14) | 6.45 (10) | 1.07× |
| Generated, high degree | 1.0 | 16.0 | 32 | 105.75 | 28.59 (16) | 14.77 (14) | **1.94×** |
| Generated, large | 16.0 | 64.0 | 8 | 2,728.99 | 573.45 (16) | 397.07 (16) | **1.44×** |

Sequential timings vary more between benchmark sessions than parallel timings. In this run, sequential DSU on the 1.0M and 1.5M graphs was much slower than in earlier runs (54.07 and 73.80 ms), so compare the parallel columns within a row rather than speedups across rows.

## Speedup over sequential DSU

The same measurements as speedups over the sequential DSU baseline: sequential time divided by parallel time, so higher is faster.

| Graph | Vertices (M) | Edges (M) | Parallel DSU | Afforest |
|---|---:|---:|---:|---:|
| Generated | 0.2 | 0.8 | 2.22× | 1.79× |
| Generated | 0.5 | 2.0 | 3.46× | 2.85× |
| Generated | 1.0 | 4.0 | 7.00× | 6.43× |
| Generated | 1.5 | 6.0 | 9.16× | 8.61× |
| California roads | 2.0 | 2.8 | 4.41× | 4.12× |
| YouTube | 1.1 | 3.0 | 4.41× | 4.73× |
| Generated, high degree | 1.0 | 16.0 | 3.70× | **7.16×** |
| Generated, large | 16.0 | 64.0 | 4.76× | **6.87×** |

These ratios depend on the sequential time, which varied more between sessions than the parallel times. The 1.0M and 1.5M rows are likely overstated, because their sequential times were unusually slow in this run.

## Findings

1. **Afforest is slightly slower than parallel DSU on low-degree graphs**
     - It's slower on the generated degree-8 graphs up to 1.5 million vertices and on California roads (0.80× to 0.94×), where the two neighbour rounds already cover most edges and the extra passes cost time. On YouTube it's slightly faster.
2.  **Afforest beats DSU as the average degree grows.** 
    - At degree 32, it's 1.94× faster than parallel DSU, which is itself 3.70× faster than sequential DSU. 
    - **Afforest also pulls ahead on the largest graph.** With 16 million vertices it's 1.44× faster. The parent array takes 64 MB, more than my device's 16 MB L3 cache, so I suspect each union costs more there. I have not profiled this.
3. **Dynamic chunks matter for skewed graphs.** 
    - Before chunking, parallel DSU took 13.20 ms on YouTube, with chunks it cut down to 9.14 ms.

## Comparison with GAP

[GAP](https://github.com/sbeamer/gapbs) is a reference benchmark suite whose CC kernel is also Afforest. **GAP uses [OpenMP](https://www.openmp.org/)**, which creates its threads once and schedules chunks of 16,384 vertices. Edgewise uses `std::jthread`, `std::barrier` and its own chunk counter, and creates its threads inside each timed call.

Both programs ran on the same files in the same sitting. Times are medians of five runs at the best thread count.

| Graph | Edgewise Afforest | GAP Afforest | GAP faster by |
|---|---:|---:|---:|
| YouTube | 6.31 ms (8) | 2.93 ms (10) | 2.15× |
| California roads | 9.86 ms (12) | 6.48 ms (12) | 1.52× |

## Raw measurements

- [Generated graphs](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_optimized_generated.csv), [high degree](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_optimized_degree32.csv), [large](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_optimized_16m.csv)
- [California roads](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_optimized_road_ca.csv), [YouTube](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_optimized_youtube.csv)
- Before the latest Afforest and concurrent DSU changes: [generated](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_chunked_generated.csv), [high degree](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_degree32.csv), [large](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_16m.csv), [California roads](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_chunked_road_ca.csv), [YouTube](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_chunked_youtube.csv)
- Before parallel DSU used chunks: [generated](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_afforest_generated.csv), [California roads](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_afforest_road_ca.csv), [YouTube](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_afforest_youtube.csv)

GAP comparison and before/after runs: [results/gap](https://github.com/Legalunicorn/edgewise/tree/main/benchmarks/results/gap).

Older measurements from before Afforest are in the [results folder](https://github.com/Legalunicorn/edgewise/tree/main/benchmarks/results/cc).

### Dataset credits

Both datasets are distributed by [SNAP](https://snap.stanford.edu/data/). The dataset pages list these source papers:

- California roads: J. Leskovec, K. Lang, A. Dasgupta and M. Mahoney, [Community Structure in Large Networks: Natural Cluster Sizes and the Absence of Large Well-Defined Clusters](https://arxiv.org/abs/0810.1355), Internet Mathematics, 2009.
- YouTube: J. Yang and J. Leskovec, [Defining and Evaluating Network Communities based on Ground-truth](https://arxiv.org/abs/1205.6233), ICDM, 2012.

The YouTube file contains the largest connected component of the network. These benchmarks use its edges, not the separate community labels.

See [Getting started](/getting-started#public-datasets) for download and run commands.
