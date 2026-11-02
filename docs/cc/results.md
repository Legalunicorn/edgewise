# CC benchmark results

::: info Under construction
This page is under construction.
:::

## Preliminary results

These results cover parallel CC using concurrent DSU. Afforest results will be added after its implementation.

Times are median milliseconds over five measured runs after warm-up on an 8-core, 16-logical-processor laptop using a Release build. Each parallel result uses the worker count with the lowest median among `1, 2, 3, 4, 6, 8, 10, 12, 14, 16`. Timing covers the complete CC call, including DSU initialization, worker startup and label collection. Loading, CSR construction and correctness checks are excluded. Every result is checked against sequential BFS outside timing.

## Public datasets

These measurements use parallel label collection. Edges are unique undirected pairs, with self-loops counted once.

| Dataset | Vertices | Edges | Sequential BFS | Sequential DSU | Parallel DSU | Workers | Speedup vs DSU |
|---|---:|---:|---:|---:|---:|---:|---:|
| [California roads](https://snap.stanford.edu/data/roadNet-CA.html) | 1,965,206 | 2,766,607 | 66.46 | 43.32 | 12.75 | 14 | 3.40× |
| [YouTube](https://snap.stanford.edu/data/com-Youtube.html) | 1,134,890 | 2,987,624 | 39.37 | 27.50 | 13.37 | 16 | 2.06× |

- Parallel DSU was **5.21× faster than sequential BFS** on California roads and **2.94× faster** on YouTube.
- These are separate benchmark sessions from the earlier measurements. They do not isolate the effect of parallel label collection: the sequential timings also changed.

Raw measurements: [California roads](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_road_ca_parallel_labels.csv), [YouTube](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_youtube_parallel_labels.csv). Earlier measurements with sequential label collection: [California roads](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_road_ca.csv), [YouTube](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_youtube.csv).

### Dataset credits

Both datasets are distributed by [SNAP](https://snap.stanford.edu/data/). The dataset pages list these source papers:

- California roads: J. Leskovec, K. Lang, A. Dasgupta and M. Mahoney, [Community Structure in Large Networks: Natural Cluster Sizes and the Absence of Large Well-Defined Clusters](https://arxiv.org/abs/0810.1355), Internet Mathematics, 2009.
- YouTube: J. Yang and J. Leskovec, [Defining and Evaluating Network Communities based on Ground-truth](https://arxiv.org/abs/1205.6233), ICDM, 2012.

The YouTube file contains the largest connected component of the network. These benchmarks use its edges, not the separate community labels. See the [README](https://github.com/Legalunicorn/edgewise#public-datasets) for download and run commands.

## Generated graphs

These saved measurements predate parallel label collection and have not been rerun with the current implementation. The inputs are generated undirected graphs with `E = 4V`, using seed `67`.

| Vertices | Sequential BFS | Sequential DSU | Parallel DSU | Workers |
|---:|---:|---:|---:|---:|
| 200,000 | 28.91 | 8.76 | 4.65 | 10 |
| 500,000 | 77.56 | 28.44 | 9.80 | 12 |
| 1,000,000 | 122.20 | 44.15 | 12.77 | 14 |
| 1,500,000 | 205.93 | 88.56 | 18.83 | 14 |

- Parallel DSU achieved **6.22×–10.94× speedup over sequential BFS** across these sizes.
- Sequential DSU was the faster sequential baseline at every size. Parallel DSU achieved **1.88×–4.70× speedup** against it.

[Raw measurements](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_all.csv)
