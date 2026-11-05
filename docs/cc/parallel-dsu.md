# Parallel CC using concurrent DSU

This implementation uses the same approach as [sequential CC using DSU](/cc/sequential-dsu), but workers claim chunks of vertices and process them using a shared [concurrent DSU](/cc/dsu/concurrent).

Here we assume an undirected graph and a positive worker count.

## Brief idea

Parallelizing our sequential DSU is quite simple, as the algorithm doesn't change much. The bulk of the complication is handled by our [ConcurrentDSU](/cc/dsu/concurrent), where we handle atomic operations in a safe manner. The remaining work is deciding which vertices each worker processes.

### Chunking
Splitting vertex by equally large ranges can result in uneven work, the work required is based on the total number of edges from from the vertices.

Instead, the vertices are cut into **chunks** of `CHUNK` (I chose `4096`) vertices, and a worker that is free claims the next unclaimed chunk from a shared atomic counter: 

```cpp
// worker claiming the next chunk
const size_t start = next_chunk.fetch_add(CHUNK);
// out of bounds, no chunks left
if (start >= size) break;

// ensure CHUNK is not larger than the
// number of vertices left to process
const size_t end = min(start + CHUNK, size);

//.... union every vertex in [start, end) with its neighbours
// loop back to the top and grab new chunk
```

`fetch_add` is atomic, so no two workers receive the same chunk. Workers that finish early simply claim more. See [Load balancing](#load-balancing) for why this helps and how to choose the chunk size.

### Pseudocode

The steps are:
1. Create one shared DSU.
2. Workers repeatedly claim a chunk of vertices and union each vertex with all its neighbours, until no chunks are left.
3. Wait at a barrier until every worker has finished its unions.
4. Each worker calls `find` for its own range of vertices to collect the component labels.
5. Join all workers before returning the labels.



## Implementation


```cpp
std::vector<int> Graph::cc_parallel_dsu(std::size_t worker_count) const {
    ConcurrentDSU dsu(size);
    std::vector<int> component(size);
    std::vector<std::jthread> workers;
    std::barrier<> unions_finished(static_cast<std::ptrdiff_t>(worker_count));

    const std::size_t CHUNK = 4096;
    std::atomic<std::size_t> next_chunk{0};

    auto work = [&](std::size_t start, std::size_t end) {
        // claim chunks of vertices until none are left
        while (true) {
            const std::size_t s = next_chunk.fetch_add(CHUNK);
            if (s >= size) break;
            const std::size_t e = std::min(s + CHUNK, size);
            for (std::size_t u = s; u < e; u++) {
                for (int v : neighbours(u)) {
                    dsu.union_set(u, v);
                }
            }
        }

        // Finish all unions before collecting component labels.
        unions_finished.arrive_and_wait();
        for (std::size_t u = start; u < end; u++) {
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

- The barrier has one participant per worker and requires `<barrier>`. It prevents labels from being collected while other workers are still merging components. 
- Workers write separate ranges of the result vector, and joining them ensures every label is ready before returning.
- Every stored edge entry is processed. 
- DSU initialization is still sequential, while edge processing and label collection are parallel.
- Label collection uses equal vertex ranges, because every vertex costs the same there.

## Load balancing

An earlier version I had for Parallel CC with DSU gave each worker an equal range of vertices for the unions as well.
This was mostly fine on randomly generated graphs I had because the edges were quite evenly distributed

However I realised this implementation wasn't as performant in real world data sets, where the distribution of edges could be uneven, such as from my the Youtube dataset.

After adding chunking, it brought parallel DSU on the Youtube dataset from from **13.20 ms** down to **9.14 ms,** roughly a **30%** improvement!

The chunk size is a tradeoff. Small chunks balance better, but every claim touches the same shared counter. Large chunks bring the imbalance back.

#### Alternative

Another option is to split vertices so that each worker gets an equal number of edges. The CSR offsets array is already a prefix sum of degrees, so each split point can be found with a binary search.

This works when the cost of each vertex is known in advance, as it is here. Dynamic chunks also handle work that is only known at runtime, such as the finish phase of [Afforest](/cc/afforest).

See the [CC benchmark results](/cc/results) for comparisons with sequential DSU and Afforest.
