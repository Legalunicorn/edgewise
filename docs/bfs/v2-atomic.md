# V2: Atomic discovery

V2 keeps the structure of [V1](/bfs/v1-mutex). Each layer splits the frontier into ranges, starts one worker per range, joins them, and combines their outputs.

The difference is how a worker claims a vertex. V1 locks one shared mutex for every check. V2 makes each distance entry atomic and claims it with a compare-and-swap (CAS), so workers claiming different vertices no longer wait for each other. Atomic operations still have a cost, mostly when workers compete for the same vertex.

## Implementation

### 1. Make the distance array atomic

```cpp
std::vector<std::atomic<int>> distance(size);
for (auto& d : distance) d.store(-1);
distance[s].store(0);
```

`std::atomic` cannot be copied, so the vector cannot be filled with `-1` in its constructor. Each entry is stored separately instead.

### 2. Claim neighbours with compare-and-swap

```cpp
for (int v : neighbours(u)) {
    int expected = -1;
    // set distance[v] to the next depth only if it is still -1
    if (distance[v].compare_exchange_strong(expected, curr_depth + 1)) {
        out.push_back(v);
    }
}
```

`compare_exchange_strong` checks and writes in one atomic step. If two workers reach `v` together, exactly one sees `-1` and succeeds, so `v` enters only one output buffer. The other worker's CAS fails, and it moves on.

### 3. Copy the distances out

```cpp
std::vector<int> result(size);
for (std::size_t i = 0; i < size; ++i) result[i] = distance[i].load();
return result;
```

The function returns plain `int` distances, so the atomic values are copied into a normal vector at the end.

Threads are still created and joined once per layer. [V3](/bfs/v3-persistent-workers) reuses them instead.

::: details Full code for parallel BFS V2

Assumes built CSR, a valid source and a positive worker count.

```cpp
#include "graph.hpp"
#include <atomic>
#include <cstddef>
#include <thread>
#include <vector>

std::vector<int> Graph::bfs_parallel_v2(int s, std::size_t worker_count) const {
    // Step 1
    std::vector<std::atomic<int>> distance(size);
    for (auto& d : distance) d.store(-1);
    distance[s].store(0);
    std::vector<int> frontier{s};
    int curr_depth = 0;

    while (!frontier.empty()) {
        std::vector<std::vector<int>> outputs(worker_count);

        // Step 2
        auto scan_range = [&](std::size_t start, std::size_t end, std::size_t id) {
            auto& out = outputs[id];
            for (std::size_t i = start; i < end; ++i) {
                const int u = frontier[i];
                for (int v : neighbours(u)) {
                    int expected = -1;
                    if (distance[v].compare_exchange_strong(expected, curr_depth + 1)) {
                        out.push_back(v);
                    }
                }
            }
        };

        std::vector<std::jthread> workers;
        for (std::size_t id = 0; id < worker_count; ++id) {
            const std::size_t start = frontier.size() * id / worker_count;
            const std::size_t end = frontier.size() * (id + 1) / worker_count;
            workers.emplace_back(scan_range, start, end, id);
        }

        // finish scanning before merging
        for (auto& worker : workers) worker.join();
        std::vector<int> next;
        for (const auto& out : outputs) {
            next.insert(next.end(), out.begin(), out.end());
        }

        frontier.swap(next);
        ++curr_depth;
    }

    // Step 3
    std::vector<int> result(size);
    for (std::size_t i = 0; i < size; ++i) result[i] = distance[i].load();
    return result;
}
```

:::
