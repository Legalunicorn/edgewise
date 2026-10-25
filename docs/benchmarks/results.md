# BFS benchmark results

::: info Under construction
This page is under construction ~ 
:::

Out of all 4 versions, V3 was the fastest version at all four graph sizes in this run. On the largest graph, it took **69.71 ms**, compared with **287.47 ms** sequentially: a **4.12× speedup**.

## Timings

All values are median milliseconds over five measured runs after warm-up. For each parallel version, I selected the worker count with the lowest median.

**Numbers in brackets are worker counts.** Sequential BFS uses one thread.

| Vertices | Undirected edges | [Sequential](/bfs/sequential) | [V1: Mutex](/bfs/v1-mutex) | [V2: Atomic](/bfs/v2-atomic) | [V3: Persistent](/bfs/v3-persistent-workers) | [V4: Parallel assembly](/bfs/v4-parallel-frontier-assembly) |
|---:|---:|---:|---:|---:|---:|---:|
| 200,000 | 800,000 | 17.02 | 51.96 <span :class="$style.workerCount">(1)</span> | 17.11 <span :class="$style.workerCount">(4)</span> | **12.30** <span :class="$style.workerCount">(8)</span> | 14.23 <span :class="$style.workerCount">(4)</span> |
| 500,000 | 2,000,000 | 72.15 | 196.17 <span :class="$style.workerCount">(1)</span> | 36.10 <span :class="$style.workerCount">(8)</span> | **25.92** <span :class="$style.workerCount">(10)</span> | 28.80 <span :class="$style.workerCount">(8)</span> |
| 1,000,000 | 4,000,000 | 178.31 | 477.30 <span :class="$style.workerCount">(1)</span> | 65.92 <span :class="$style.workerCount">(14)</span> | **47.63** <span :class="$style.workerCount">(16)</span> | 53.71 <span :class="$style.workerCount">(14)</span> |
| 1,500,000 | 6,000,000 | 287.47 | 764.75 <span :class="$style.workerCount">(1)</span> | 90.06 <span :class="$style.workerCount">(14)</span> | **69.71** <span :class="$style.workerCount">(14)</span> | 73.64 <span :class="$style.workerCount">(14)</span> |

Speedup is the sequential median divided by the parallel median. The worker counts above are the best among those tested, not a general recommendation for every graph.

## Speedup

Compared with sequential BFS on the same graph. Brackets show the selected worker count; values below **1.00×** mean slower than sequential.

| Vertices | Undirected edges | V1: Mutex | V2: Atomic | V3: Persistent | V4: Parallel assembly |
|---:|---:|---:|---:|---:|---:|
| 200,000 | 800,000 | 0.33× <span :class="$style.workerCount">(1)</span> | 0.99× <span :class="$style.workerCount">(4)</span> | **1.38×** <span :class="$style.workerCount">(8)</span> | 1.20× <span :class="$style.workerCount">(4)</span> |
| 500,000 | 2,000,000 | 0.37× <span :class="$style.workerCount">(1)</span> | 2.00× <span :class="$style.workerCount">(8)</span> | **2.78×** <span :class="$style.workerCount">(10)</span> | 2.51× <span :class="$style.workerCount">(8)</span> |
| 1,000,000 | 4,000,000 | 0.37× <span :class="$style.workerCount">(1)</span> | 2.71× <span :class="$style.workerCount">(14)</span> | **3.74×** <span :class="$style.workerCount">(16)</span> | 3.32× <span :class="$style.workerCount">(14)</span> |
| 1,500,000 | 6,000,000 | 0.38× <span :class="$style.workerCount">(1)</span> | 3.19× <span :class="$style.workerCount">(14)</span> | **4.12×** <span :class="$style.workerCount">(14)</span> | 3.90× <span :class="$style.workerCount">(14)</span> |

[Raw measurements](https://github.com/Legalunicorn/edgewise/blob/main/benchmarks/results/bfs/bfs_comparison_v1-v4_last.csv)

## Observations

#### v1 did not scall well (as expected)
Its best result used only one worker at every size, and was still slower than sequential BFS. Every discovery check passes through the same shared mutex, which is obviously a huge bottle neck. 

This was expected as the main purpose of V1 was transition to parallel implementation and introducing worker splitting.


#### V2 improved the timing significantly.

Replacing the mutex with atomic discovery reached **3.19× speedup** on the largest graph. At 200,000 vertices, it was roughly equal to sequential BFS.

#### V3 improved on V2 at every size
It reuses workers and private output buffers. It also adds a visited check before compare-and-swap and changes the initialization stores, so the difference does not isolate thread reuse alone.


#### V4 was slower than V3 at every size

On the largest graph, both selected 14 workers, but V4 took **73.64 ms** versus **69.71 ms**, about **6% longer**. I was surprised by these results, and have yet to investigate which component was responsible for the additional costs.

V3's speedup increased from **1.38×** on the smallest graph to **4.12×** on the largest. Larger inputs can spread fixed parallel overhead over more work and change cache behaviour. 


## Test conditions

- Release build on my laptop: **8 physical cores, 16 logical processors**.
- One generated graph per size: an undirected binary-tree backbone plus random extra edges, using **seed 67**. Each undirected edge is stored in both directions.
- Tested worker counts: **1, 2, 3, 4, 6, 8, 10, 12, 14, 16**. Every implementation uses the same graph at each size.
- Timings cover the complete BFS call, including initialization, thread setup and output construction. Graph generation, CSR construction and correctness checks are outside the timer.
- Every traversal is compared with sequential distances. Implementations run in a fixed order.

These results cover one graph family, one seed and one machine. Different graph structures or hardware may change the ranking and worker count. I currently use **V3 as the preferred parallel BFS implementation**.

<style module>
.workerCount {
  color: var(--vp-c-text-2);
  font-size: 0.85em;
  font-weight: 400;
  white-space: nowrap;
}
</style>
