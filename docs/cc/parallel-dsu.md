# Parallel CC using concurrent DSU

::: info Under construction
This page is under construction.
:::

This implementation uses the same approach as [sequential CC using DSU](/cc/sequential-dsu), but workers process separate vertex ranges using a shared [concurrent DSU](/cc/dsu/concurrent).

Here we assume an undirected graph and a positive worker count.

## Brief idea

To parallelize our sequential DSU version directly is fairly simple. The bulk of the complication is handled by our [ConcurrentDSU](/cc/dsu/concurrent), where we handle atomic operations in a safe manner.

The steps is as such:
1. Create one shared DSU and divide the vertices evenly among workers.
2. Each worker unions its vertices with all their neighbours.
3. Wait at a barrier until every worker has finished its unions.
4. Each worker calls `find` for its own vertices to collect the component labels.
5. Join all workers before returning the labels.



## Implementation

```cpp
std::vector<int> Graph::cc_parallel_dsu(std::size_t worker_count) const {
    ConcurrentDSU dsu(size);
    std::vector<int> component(size);
    std::vector<std::jthread> workers;
    std::barrier<> unions_finished(static_cast<std::ptrdiff_t>(worker_count));

    // Process this worker's vertices using the shared DSU.
    auto work = [&](std::size_t start, std::size_t end) {
        for (int u = start; u < end; u++) {
            for (int v : neighbours(u)) {
                dsu.union_set(u, v);
            }
        }

        // Finish all unions before collecting component labels.
        unions_finished.arrive_and_wait();
        for (int u = start; u < end; u++) {
            component[u] = dsu.find(u);
        }
    };

    for (std::size_t w = 0; w < worker_count; w++) {
        const std::size_t start = size * w / worker_count;
        const std::size_t end = size * (w + 1) / worker_count;
        workers.emplace_back(work, start, end);
    }

    // Wait for every worker to finish writing its labels.
    for (auto& worker : workers) worker.join();
    return component;
}
```

The barrier has one participant per worker and requires `<barrier>`. It prevents labels from being collected while other workers are still merging components. Workers write separate ranges of the result vector, and joining them ensures every label is ready before returning.

The ranges divide vertices evenly, but vertices with more neighbours give their workers more work. Every stored edge entry is processed. DSU initialization is still sequential; edge processing and label collection are parallel.

See the [CC benchmark results](/cc/results) for comparisons against both sequential implementations.

## Improvements
This has not yet been implemented, but a possible optimization is to not split vertices evenly among workers, but instead split by the number of edges to process instead. 

This would be beneficial for very unbalanced graphs where a few consecutive vertices holds an disproportionately large number of edges, creating a bottle neck for the algorithm.
