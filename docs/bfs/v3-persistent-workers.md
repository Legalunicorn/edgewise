# V3: Persistent workers

[V2](/bfs/v2-atomic) creates and joins its workers for every layer. V3 creates them once per BFS call and reuses them across layers.

Two barriers keep the workers and the main thread in step:

1. **`start_round`:** workers wait until the main thread has the frontier ready.
2. **`end_round`:** the main thread waits until every worker has finished scanning.

Between `end_round` and the next `start_round`, the main thread combines the output buffers and advances the frontier while the workers wait. It still does this sequentially; [V4](/bfs/v4-parallel-frontier-assembly) parallelizes it.

V3 also checks whether a vertex is already visited before attempting compare-and-swap, so its comparison with V2 includes both changes.

## Walkthrough

![Animation of V3 BFS with three workers on a 21-vertex graph](/animations/parallel-bfs-v3.svg)

The animation follows V3 on a small directed graph with three workers.

- **Split:** the strip at the top is the current frontier. Worker `id` scans the range `[n * id / W, n * (id + 1) / W)`, and the outlined cell is the vertex being scanned.
- **Claim:** a worker skips neighbours that are already visited, and claims the rest with compare-and-swap. At level 1, W0 and W1 reach vertex 5 together, and W0's claim fails.
- **Barrier:** the timeline shows workers that finish early idling until the barrier. The main thread then joins the output buffers in worker order, and the frontier is re-split, so a vertex can change worker between layers.
- **Done:** finished vertices turn grey, and the last frame shows the BFS tree.

::: tip Note
Generative AI was used to create the visualizations for this project.
:::

## Implementation

### 1. Create two barriers that include the main thread

```cpp
// the workers plus the main thread
const auto participants = static_cast<std::ptrdiff_t>(worker_count + 1);
std::barrier<> start_round(participants);
std::barrier<> end_round(participants);
bool done = false;
```

A barrier resets after every crossing, so the same two objects are reused for every layer.

### 2. Skip visited vertices before compare-and-swap

```cpp
for (int v : neighbours(u)) {
    // already claimed, so the CAS would fail anyway
    if (distance[v].load() != -1) continue;
    int expected = -1;
    if (distance[v].compare_exchange_strong(expected, curr_depth + 1)) {
        out.push_back(v);
    }
}
```

A load only reads the value, while a CAS needs exclusive access to it even when it fails. Most neighbours in later layers are already visited, so the check avoids many failing CAS operations.

### 3. Loop each worker over the layers

```cpp
auto worker_loop = [&](std::size_t id) {
    while (true) {
        // wait until the frontier for this layer is ready
        start_round.arrive_and_wait();
        if (done) break;
        outputs[id].clear();
        const std::size_t start = frontier.size() * id / worker_count;
        const std::size_t end = frontier.size() * (id + 1) / worker_count;
        scan_range(start, end, id);
        // tell the main thread this worker has finished the layer
        end_round.arrive_and_wait();
    }
};
```

Each worker recomputes its range every layer, because the frontier size changes.

### 4. Drive the layers from the main thread

```cpp
while (!frontier.empty()) {
    // release the workers into this layer
    start_round.arrive_and_wait();
    // wait for every worker to finish scanning
    end_round.arrive_and_wait();

    std::vector<int> next;
    for (const auto& out : outputs) {
        next.insert(next.end(), out.begin(), out.end());
    }
    frontier.swap(next);
    ++curr_depth;
}

// release the workers one last time, so they see done and exit
done = true;
start_round.arrive_and_wait();
for (auto& worker : workers) worker.join();
```

`frontier`, `curr_depth` and `done` are plain variables. The main thread only writes them while the workers wait between barriers, and a barrier makes those writes visible to every worker after the crossing.

::: details Full code for parallel BFS V3

Assumes built CSR, a valid source and a positive worker count.

```cpp
#include "graph.hpp"
#include <atomic>
#include <barrier>
#include <cstddef>
#include <thread>
#include <vector>

std::vector<int> Graph::bfs_parallel_v3(int s, std::size_t worker_count) const {
    std::vector<std::atomic<int>> distance(size);
    for (auto& d : distance) d.store(-1);
    distance[s].store(0);
    std::vector<int> frontier{s};
    std::vector<std::vector<int>> outputs(worker_count);
    int curr_depth = 0;

    // Step 1
    const auto participants = static_cast<std::ptrdiff_t>(worker_count + 1);
    std::barrier<> start_round(participants);
    std::barrier<> end_round(participants);
    bool done = false;

    // Step 2
    auto scan_range = [&](std::size_t start, std::size_t end, std::size_t id) {
        auto& out = outputs[id];
        for (std::size_t i = start; i < end; ++i) {
            const int u = frontier[i];
            for (int v : neighbours(u)) {
                if (distance[v].load() != -1) continue;
                int expected = -1;
                if (distance[v].compare_exchange_strong(expected, curr_depth + 1)) {
                    out.push_back(v);
                }
            }
        }
    };

    // Step 3
    auto worker_loop = [&](std::size_t id) {
        while (true) {
            start_round.arrive_and_wait();
            if (done) break;
            outputs[id].clear();
            const std::size_t start = frontier.size() * id / worker_count;
            const std::size_t end = frontier.size() * (id + 1) / worker_count;
            scan_range(start, end, id);
            end_round.arrive_and_wait();
        }
    };

    std::vector<std::jthread> workers;
    for (std::size_t id = 0; id < worker_count; ++id) {
        workers.emplace_back(worker_loop, id);
    }

    // Step 4
    while (!frontier.empty()) {
        start_round.arrive_and_wait();
        end_round.arrive_and_wait();
        std::vector<int> next;
        for (const auto& out : outputs) {
            next.insert(next.end(), out.begin(), out.end());
        }
        frontier.swap(next);
        ++curr_depth;
    }
    done = true;
    start_round.arrive_and_wait();
    for (auto& worker : workers) worker.join();

    std::vector<int> result(size);
    for (std::size_t i = 0; i < size; ++i) result[i] = distance[i].load();
    return result;
}
```

:::

