# PageRank benchmark results

On the largest graph, parallel PageRank took **429.22 ms**, compared with **1,885.92 ms** sequentially: a **4.39× speedup** using 16 workers.

## Timings

Times are median milliseconds over five measured runs after warm-up. For each graph, I selected the parallel worker count with the lowest median.

| Vertices | Undirected edges | Sequential | Parallel | Workers | Speedup |
|---:|---:|---:|---:|---:|---:|
| 200,000 | 800,000 | 71.83 | 27.79 | 8 | 2.58× |
| 500,000 | 2,000,000 | 274.67 | 77.23 | 6 | 3.56× |
| 1,000,000 | 4,000,000 | 1,464.01 | 344.65 | 14 | 4.25× |
| 1,500,000 | 6,000,000 | 1,885.92 | 429.22 | 16 | **4.39×** |

Speedup is the sequential median divided by the parallel median.

[Raw measurements](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/pagerank/pagerank_all.csv)

## Test conditions

- Release build on an **8-core, 16-logical-processor laptop**.
- One generated graph per size: an undirected binary-tree backbone plus random extra edges, using **seed 67**.
- Worker counts: **1, 2, 3, 4, 6, 8, 10, 12, 14, 16**.
- Both implementations use damping `0.85`, convergence tolerance `1e-10`, and an iteration limit of `100`.
- One warm-up sweep followed by five measured sweeps. Sequential runs first, followed by parallel worker counts in ascending order.
- Timing includes the complete PageRank call, including initialization and worker setup. Graph generation, CSR construction, and correctness checks are excluded.

Every measured result is checked against sequential scores with an absolute tolerance of `1e-10`. Scores must also be finite, nonnegative, and sum to 1 within `1e-8`.

## Observations

- Speedup increased from **2.58×** to **4.39×** across these sizes. The best worker count varied between graphs.


- The parallel implementation was slower than sequential with one worker at every size.

- These measurements cover one generated graph family, one seed, and one machine.
- The graphs have no dangling vertices; dangling handling is covered by correctness tests.
- Actual iteration counts are not recorded, so these timings do not establish the cause of differences between graph sizes.
