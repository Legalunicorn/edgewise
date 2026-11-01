# CC benchmark results

::: info Under construction
This page is under construction.
:::

## Preliminary results

These results cover parallel CC using concurrent DSU. Afforest results will be added after its implementation.

Times are median milliseconds over five measured runs after warm-up. Each parallel result uses the worker count with the lowest median. The inputs are generated undirected graphs with `E = 4V`, using seed `67`.

| Vertices | Sequential BFS | Sequential DSU | Parallel DSU | Workers |
|---:|---:|---:|---:|---:|
| 200,000 | 28.91 | 8.76 | 4.65 | 10 |
| 500,000 | 77.56 | 28.44 | 9.80 | 12 |
| 1,000,000 | 122.20 | 44.15 | 12.77 | 14 |
| 1,500,000 | 205.93 | 88.56 | 18.83 | 14 |

- Parallel DSU achieved **6.22×–10.94× speedup over sequential BFS** across these sizes.
- Sequential DSU was the faster sequential baseline at every size. Parallel DSU achieved **1.88×–4.70× speedup** against it.

[Raw measurements](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/cc/cc_all.csv)
