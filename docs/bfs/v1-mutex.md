::: warning Note to reader
This page is currently a work in progress = )
:::
# Parallel BFS V1: Mutex discovery

A key idea in parallel BFS is that different frontiers must be processed in sequence (1st frontier, then 2nd, then 3rd...), but work within one frontier can be done in parallel.

Every vertex in a frontier has the same distance from the source. Newly discovered vertices belong to the next distance layer, so the order we process vertices within one frontier does not affect the returned distances.


The main challenges are claiming newly discovered vertices safely, coordinating workers between layers and collecting their results.

<!-- ## Common structure -->
<!---->
<!-- Each parallel implementation divides the current frontier into contiguous ranges. Workers scan their assigned vertices and examine their neighbours. -->
<!---->
<!-- For example, suppose the frontier contains 6 vertices, and we have 3 workers. Then,  -->
<!-- - worker 1: takes vertices 1, 2 -->
<!-- - worker 2: takes vertices 3, 4 -->
<!-- - worker 3: takes vertices 5, 6 -->
<!---->
<!-- A worker records successful discoveries in its own output vector. These private buffers allow workers to append results without concurrently modifying one shared vector. -->
<!---->
<!-- The buffers are then combined into the next frontier. -->
<!---->
<!-- > Note: Equal numbers of vertices do not necessarily mean equal work: one assigned vertex may have far more neighbours than another. This will be covered in v5. -->

## Implementation


This version introduces workers to the BFS implementation, allowing them to each take a portion of a frontier. However, it also requires the use of a mutex when different workers are checking neighbours in parallel.

### 1. Declare a mutex before BFS
```cpp
// declare other bfs variables...
std::mutex discovery_mutex;
```


### 2. Create lambdas to scan a range and claim vertices
Each worker scans frontier indices in `[start, end)` using `scan_range`. It calls `try_discover(v)` to claim each unvisited neighbour. Steps 2–5 run once per frontier, inside `while (!frontier.empty())`.


```cpp
// Tries to discover a vertex, v
// returns TRUE if worker managed to discover v, FALSE otherwise.
auto try_discover = [&](int v) -> bool {
    std::lock_guard<std::mutex> guard(discovery_mutex);
    if (distance[v] == -1){ // -1 means unvisited
        distance[v] = curr_depth + 1;
        return true;
    }
    return false;
    // lock_guard is automatically destroyed when function returns
};
```
::: info Why the use of lock_guard?
The lock protects both the check and the update. Only one worker can change a vertex from unvisited to discovered and add it to its output.
:::


```cpp
// Each worker gets its own output vector for this frontier.
std::vector<std::vector<int>> outputs(worker_count);

// start, end: scan frontier indices in [start, end), excluding end.
// id: zero-based worker ID.
auto scan_range =[&](std::size_t start, std::size_t end, std::size_t id) {
    auto& out = outputs[id];
    for (std::size_t i = start; i < end; i++){
        const int u = frontier[i];
        for (int v: neighbours(u)){
            // worker appends discovered vertex to its own result list
            if (try_discover(v))  out.push_back(v);
        }
    }
};
```


### 3. Create workers and assign their ranges


After defining the lambda, start the workers:

```cpp
std::vector<std::jthread> workers; // No threads started yet.
for (std::size_t id = 0; id < worker_count; ++id) {
    const std::size_t start = frontier.size() * id / worker_count; // Inclusive
    const std::size_t end = frontier.size() * (id + 1) / worker_count; // Exclusive
    workers.emplace_back(scan_range, start, end, id); // Start scan_range(start, end, id)
}
```

The ranges divide the frontier without gaps or overlap. For 10 vertices and 3 workers, they are `[0, 3)`, `[3, 6)` and `[6, 10)`.

### 4. Join workers and combine their results

The main thread waits for all workers before reading their outputs:

```cpp
for (auto& worker : workers) {
    worker.join(); // Wait for this worker to finish.
}

std::vector<int> next; // Collect the next frontier.
for (const auto& out : outputs) {
    next.insert(next.end(), out.begin(), out.end()); // Append each worker's output.
}
```

The workers already run concurrently; `join()` only waits. The main thread then combines their outputs sequentially.

### 5. Advance the frontier

```cpp
frontier.swap(next); // Use the collected vertices as the new frontier.
++curr_depth;        // Advance to the next distance.
```

Repeat until `frontier` is empty. V1 creates new workers for every layer and still serializes discovery checks through one mutex.

::: details Full code for parallel BFS V1

Assumes built CSR, a valid source and a positive worker count.

```cpp
#include "graph.hpp"
#include <cstddef>
#include <mutex>
#include <thread>
#include <vector>

std::vector<int> Graph::bfs_parallel_v1(int s, std::size_t worker_count) const {
    std::vector<int> distance(size, -1);
    distance[s] = 0;
    std::vector<int> frontier{s};
    int curr_depth = 0;

    // Step 1
    std::mutex discovery_mutex;

    while (!frontier.empty()) {

        // Step 2
        auto try_discover = [&](int v) -> bool {
            std::lock_guard<std::mutex> guard(discovery_mutex);
            if (distance[v] == -1) {
                distance[v] = curr_depth + 1;
                return true;
            }
            return false;
        };
        std::vector<std::vector<int>> outputs(worker_count);
        auto scan_range = [&](std::size_t start, std::size_t end, std::size_t id) {
            auto& out = outputs[id];
            for (std::size_t i = start; i < end; ++i) {
                const int u = frontier[i];
                for (int v : neighbours(u)) {
                    if (try_discover(v))  out.push_back(v);
                }
            }
        };

        // Step 3
        std::vector<std::jthread> workers;
        for (std::size_t id = 0; id < worker_count; ++id) {
            const std::size_t start = frontier.size() * id / worker_count;
            const std::size_t end = frontier.size() * (id + 1) / worker_count;
            // start the worker
            workers.emplace_back(scan_range, start, end, id);
        }

        // Step 4
        for (auto& worker : workers) {
            worker.join(); // Finish scanning before merging.
        }
        std::vector<int> next;
        for (const auto& out : outputs) {
            next.insert(next.end(), out.begin(), out.end());
        }

        // Step 5
        frontier.swap(next);
        ++curr_depth;
    }
    return distance;
}
```

:::
