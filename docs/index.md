
# Edgewise

Edgewise is a personal project exploring parallel graph algorithms in C++20. It compares sequential and parallel implementations, examining how synchronization, graph storage and work distribution affect performance.

So far, I’ve implemented sequential and parallel breadth-first search (BFS) and PageRank. I plan to explore parallel connected components (CC) for undirected graphs later.


## BFS results

I wrote my parallel BFS in versions, each introducing a new possible optimization. Details of which are discussed in [the BFS tutorials](/bfs/v1-mutex).

A brief summary of the parallel versions implemented:
- V1: protect vertex discovery with a shared mutex.
- V2: replace discovery mutex with atomic compare-and-swap.
- V3: reuse worker threads across all BFS layers.
- V4: assemble the next frontier in parallel.

### Table for BFS results

<div :class="$style.bfsResults">

| Vertices | Edges (undirected) | Sequential | Parallel | Workers | Speedup |
|---:|---:|---:|---:|---:|---:|
| 200,000 | 800,000 | 17.02 ms | 12.30 ms | 8 | 1.38× |
| 500,000 | 2,000,000 | 72.15 ms | 25.92 ms | 10 | 2.78× |
| 1,000,000 | 4,000,000 | 178.31 ms | 47.63 ms | 16 | 3.74× |
| 1,500,000 | 6,000,000 | 287.47 ms | 69.71 ms| 14 | **4.12×**|

</div>

<style module>
.bfsResults tbody tr:last-child td {
  background-color: var(--vp-c-green-soft); /* Subtle green background. */
  font-weight: 600; /* Slightly emphasize the entire row. */
}
.bfsResults thead th {
  background-color: var(--vp-c-bg-alt); /* Theme-aware header background. */
  color: var(--vp-c-text-1); /* Strong text contrast in either mode. */
  font-weight: 700;
  border-bottom: 2px solid var(--vp-c-divider); /* Subtle separation. */
}
</style>

::: info Table notes
- **Sequential:** single-threaded BFS using CSR storage.
- **Parallel**: Version 3 of my parallel implementation, which currently performs the best.
- Graphs are undirected, connected, and generated from a binary-tree backbone, with random extra edges using a random seed 67.
- Measurements use a Release build on an 8-core, 16-logical-processor laptop.
:::

Times are medians of five measured repetitions after warm-up. For each graph, the parallel result uses the worker count with the lowest median. Speedup is the sequential median divided by the parallel median. See the [benchmark methodology](/benchmarks/methodology) and [results](/benchmarks/results) for details.

## PageRank results

The parallel implementation splits vertices among persistent workers and combines their results after each round.

<div :class="$style.bfsResults">

| Vertices | Edges (undirected) | Sequential | Parallel | Workers | Speedup |
|---:|---:|---:|---:|---:|---:|
| 200,000 | 800,000 | 71.83 ms | 27.79 ms | 8 | 2.58× |
| 500,000 | 2,000,000 | 274.67 ms | 77.23 ms | 6 | 3.56× |
| 1,000,000 | 4,000,000 | 1,464.01 ms | 344.65 ms | 14 | 4.25× |
| 1,500,000 | 6,000,000 | 1,885.92 ms | 429.22 ms | 16 | **4.39×** |

</div>

Times are medians of five measured runs after warm-up, using the worker count with the lowest parallel median. These use the same generated graph sizes and laptop as the BFS results above. Both PageRank implementations use damping `0.85`, tolerance `1e-10`, and an iteration limit of `100`.

See the [PageRank results](/pagerank/results) for measurement details and [parallel implementation](/pagerank/parallel) for the code.

## Explore

- [Sequential BFS](/bfs/sequential): the algorithm, returned distances and frontier structure.
- [Parallel BFS](/bfs/v1-mutex): the purpose and tradeoffs of each implementation.
- [PageRank](/pagerank/): the algorithm, implementations and results.
- [Graph storage](/graph-storage.md): the shared CSR representation.
- [Getting started](/getting-started): build, test and run the project.
