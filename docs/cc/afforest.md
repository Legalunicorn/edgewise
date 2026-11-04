# Afforest


Afforest is a parallel connected components algorithm by Sutton, Ben-Nun and Barak, from the paper *Optimizing Parallel Graph Connectivity Computation via Subgraph Sampling* (IPDPS 2018, [code](https://github.com/michaelsutton/afforest)). The [GAP benchmark suite](https://github.com/sbeamer/gapbs) uses it for its CC kernel.

It uses the same [concurrent DSU](/cc/dsu/concurrent) as [parallel CC using concurrent DSU](/cc/parallel-dsu). The difference is which edges it processes.

## Brief idea

Parallel DSU unions *every edge*. Many of those unions find that both endpoints are already in the same component, so they do no useful work. Afforest tries to skip them.

#### Steps
1. Union each vertex with only its first neighbour, then with its second neighbour. In many graphs this already joins most vertices into one large component.
2. Guess which component is the largest: look at a *small* random sample of vertices and take the component that appears most often.
3. Process the remaining neighbours the same way as in [Parallel CC using DSU](/cc/parallel-dsu), but **skip every vertex that is already in the largest component.**


Between steps, a **compress** runs over every vertex, so that its `parent` in the DSU points directly at its root.

This keeps trees shallow, and it means a vertex's label can be read with a single load.

## Why skipping is safe

Let $C$ be the guessed largest component, and take any edge `u-v`:

- If `u` and `v` are both in $C$, they are already connected. The edge adds nothing.
- If neither is in $C$, both endpoints process the edge.
- If only `u` is in $C$, then `v` processes the edge. Each undirected edge is stored in both directions, so `u` is in `v`'s neighbour list.

So no edge that could merge two components is skipped. If $C$ is guessed wrongly, the result is still correct, only slower.

## Phases

All phases share one `ConcurrentDSU`. Workers wait at a barrier after each phase.

| Phase | Work |
|---|---|
| 1. Round 0 | Each worker links its vertices with their first neighbour. |
| 2. Compress | Each worker calls `find` on its vertices. |
| 3. Round 1 | Each worker links its vertices with their second neighbour. |
| 4. Compress | Same as phase 2. |
| 5. Sample | The main thread samples 1024 random vertices and picks the most common label `c`. |
| 6. Finish | Workers link the remaining neighbours of every vertex not in `c`. |
| 7. Labels | Each worker writes `find(v)` for its vertices into the result. |

## Implementation

### Helper methods
The helpers are free functions in an anonymous namespace inside `cc_parallel_afforest.cpp`. The ones that process vertices take a range, so the same helper works for a worker's static range or for a chunk.

#### 1. Link one neighbour per vertex

```cpp
// link each vertex in [start, end) with its r-th neighbour, if it has one
void link_round(const Graph& g, ConcurrentDSU& dsu, std::size_t r,
                std::size_t start, std::size_t end) {
    for (std::size_t u = start; u < end; u++) {
        if (g.outdegree(u) <= r) continue;
        dsu.union_set(u, g.neighbours(u)[r]);
    }
}
```

`union_set` is the same CAS-based union from the concurrent DSU. Compress is a loop that calls `dsu.find(u)`, since `find` already rewrites parents to point at the root. `fill_results` is the same loop, but stores each `find(u)` in the result vector.

#### 2. Sample the largest component

The main thread runs this while every worker waits at the barrier, so the parent array does not change during sampling.

```cpp
int sample_frequent_label(ConcurrentDSU& dsu, std::uint32_t seed, std::size_t n) {
    // fixed seed, so every run samples the same vertices
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> pick(0, n - 1);
    std::unordered_map<int, int> count;
    for (int i = 0; i < 1024; i++) {
        count[dsu.find(pick(rng))]++;
    }

    int c = 0;
    int best = 0;
    for (const auto& [label, freq] : count) {
        if (freq > best) {
            best = freq;
            c = label;
        }
    }
    return c;
}
```

If the largest component holds most of the vertices, most samples land in it, so its label wins the count.

#### 3. Link the remaining neighbours

```cpp
// neighbours 0 and 1 are linked in the rounds, so the rest start at index 2
constexpr std::size_t NEIGHBOUR_ROUNDS = 2;

// link the leftover neighbours of vertices outside component c
void link_remaining(const Graph& g, ConcurrentDSU& dsu,
                    std::size_t start, std::size_t end, int c) {
    for (std::size_t u = start; u < end; u++) {
        if (g.outdegree(u) <= NEIGHBOUR_ROUNDS) continue;
        // skip vertices already in c
        if (dsu.parent[u].load() == c) continue;
        for (int v : g.neighbours(u).subspan(NEIGHBOUR_ROUNDS)) {
            dsu.union_set(u, v);
        }
    }
}
```

::: info Why check the direct parent and not `find(u)`?
During this phase the root of `c` can change. If a component with a smaller root ID merges into it, `c` itself gets a new parent. 

After that, `find(u)` returns the new root for every vertex in the large component, so a `find(u) == c` check would stop skipping anything.

Linking only changes the parents of roots, so most vertices in the large component keep `parent[u] == c`. The check is also always safe, because `parent[u] == c` means `u` is connected to `c`.
:::

#### 4. Coordinate the phases

One barrier is shared by the workers and the main thread, so it has `worker_count + 1` participants. A barrier resets after each crossing, so the same object is reused for every phase.

```cpp
std::barrier<> sync(worker_count + 1);

auto work = [&](std::size_t id) {
    const std::size_t start = size * id / worker_count;
    const std::size_t end = size * (id + 1) / worker_count;

    // crossings 1 to 4: two rounds, each followed by a compress
    link_round(*this, dsu, 0, start, end);
    sync.arrive_and_wait();
    compress(dsu, start, end);
    sync.arrive_and_wait();
    link_round(*this, dsu, 1, start, end);
    sync.arrive_and_wait();
    compress(dsu, start, end);
    sync.arrive_and_wait();

    // crossing 5: wait while the main thread samples
    sync.arrive_and_wait();
    // the finish phase goes here, see the next section
    // crossing 6: wait until every worker has finished linking
    sync.arrive_and_wait();

    fill_results(component, dsu, start, end);
};

for (std::size_t w = 0; w < worker_count; w++) workers.emplace_back(work, w);

// crossings 1 to 4
for (int i = 0; i < 4; i++) sync.arrive_and_wait();
c = sample_frequent_label(dsu, 67, size);
// crossing 5: releases the workers into the finish phase
sync.arrive_and_wait();
// crossing 6: all links are done
sync.arrive_and_wait();
for (auto& w : workers) w.join();
```

The main thread and every worker must arrive the same number of times, or the program deadlocks. The barrier also makes the main thread's write to `c` visible to the workers after crossing 5.

Only two crossings are needed for correctness. Crossing 5 lets workers see `c`, and crossing 6 makes sure all links finish before labels are collected. The others keep trees shallow and give the sample settled labels.

#### 5. Balance the finish phase with chunks

This is the same chunk loop as in [Parallel CC using DSU](/cc/parallel-dsu), except that `link_remaining` links only from the third neighbour onward, and skips vertices in $C$.

```cpp
const std::size_t CHUNK = 4096;
std::atomic<std::size_t> next_chunk{0};

// inside each worker, between crossings 5 and 6
while (true) {
    const std::size_t s = next_chunk.fetch_add(CHUNK);
    if (s >= size) break;
    const std::size_t e = std::min(s + CHUNK, size);
    link_remaining(*this, dsu, s, e, c);
}
```

::: details Full implementation

```cpp
#include "graph.hpp"
#include "dsu_concurrent.hpp"
#include <algorithm>
#include <atomic>
#include <barrier>
#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <vector>

namespace {

// neighbours 0 and 1 are linked in the rounds, so the rest start at index 2
constexpr std::size_t NEIGHBOUR_ROUNDS = 2;

// link each vertex in [start, end) with its r-th neighbour, if it has one
void link_round(const Graph& g, ConcurrentDSU& dsu, std::size_t r,
                std::size_t start, std::size_t end) {
    for (std::size_t u = start; u < end; u++) {
        if (g.outdegree(u) <= r) continue;
        dsu.union_set(u, g.neighbours(u)[r]);
    }
}

// point every vertex in [start, end) straight at its root
void compress(ConcurrentDSU& dsu, std::size_t start, std::size_t end) {
    for (std::size_t u = start; u < end; u++) {
        dsu.find(u);
    }
}

// store the root of every vertex in [start, end) as its label
void fill_results(std::vector<int>& component, ConcurrentDSU& dsu,
                  std::size_t start, std::size_t end) {
    for (std::size_t u = start; u < end; u++) {
        component[u] = dsu.find(u);
    }
}

// the most common label among 1024 random vertices
int sample_frequent_label(ConcurrentDSU& dsu, std::uint32_t seed, std::size_t n) {
    // fixed seed, so every run samples the same vertices
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> pick(0, n - 1);
    std::unordered_map<int, int> count;
    for (int i = 0; i < 1024; i++) {
        count[dsu.find(pick(rng))]++;
    }

    int c = 0;
    int best = 0;
    for (const auto& [label, freq] : count) {
        if (freq > best) {
            best = freq;
            c = label;
        }
    }
    return c;
}

// link the leftover neighbours of vertices outside component c
void link_remaining(const Graph& g, ConcurrentDSU& dsu,
                    std::size_t start, std::size_t end, int c) {
    for (std::size_t u = start; u < end; u++) {
        if (g.outdegree(u) <= NEIGHBOUR_ROUNDS) continue;
        // skip vertices already in c
        if (dsu.parent[u].load() == c) continue;
        for (int v : g.neighbours(u).subspan(NEIGHBOUR_ROUNDS)) {
            dsu.union_set(u, v);
        }
    }
}

}  // namespace

std::vector<int> Graph::cc_parallel_afforest(std::size_t worker_count) const {
    if (size == 0) return {};
    if (worker_count == 0) {
        throw std::invalid_argument("Worker count must be positive");
    }

    ConcurrentDSU dsu(size);
    std::vector<int> component(size);
    std::vector<std::jthread> workers;
    workers.reserve(worker_count);

    // the main thread is a participant as well
    std::barrier<> sync(worker_count + 1);

    const std::size_t CHUNK = 4096;
    std::atomic<std::size_t> next_chunk{0};

    // label of the guessed largest component, written by the main thread before crossing 5
    int c = 0;

    auto work = [&](std::size_t id) {
        const std::size_t start = size * id / worker_count;
        const std::size_t end = size * (id + 1) / worker_count;

        // crossings 1 to 4: two rounds, each followed by a compress
        link_round(*this, dsu, 0, start, end);
        sync.arrive_and_wait();
        compress(dsu, start, end);
        sync.arrive_and_wait();
        link_round(*this, dsu, 1, start, end);
        sync.arrive_and_wait();
        compress(dsu, start, end);
        sync.arrive_and_wait();

        // crossing 5: wait while the main thread samples
        sync.arrive_and_wait();

        // finish phase: claim chunks of vertices until none are left
        while (true) {
            const std::size_t s = next_chunk.fetch_add(CHUNK);
            if (s >= size) break;
            const std::size_t e = std::min(s + CHUNK, size);
            link_remaining(*this, dsu, s, e, c);
        }

        // crossing 6: wait until every worker has finished linking
        sync.arrive_and_wait();

        fill_results(component, dsu, start, end);
    };

    for (std::size_t w = 0; w < worker_count; w++) {
        workers.emplace_back(work, w);
    }

    // crossings 1 to 4
    for (int i = 0; i < 4; i++) sync.arrive_and_wait();
    c = sample_frequent_label(dsu, 67, size);
    // crossing 5: releases the workers into the finish phase
    sync.arrive_and_wait();
    // crossing 6: all links are done
    sync.arrive_and_wait();

    for (auto& w : workers) w.join();
    return component;
}
```

:::

## When Afforest helps

Afforest saves edge work, but adds a few extra passes over all vertices. It pulls ahead when there is a lot of edge work to skip:

- **Higher average degree.** Two rounds read two neighbours per vertex. The rest of each list is skipped for vertices in the large component.
- **Larger graphs.** On 16 million vertices it was 1.62× faster. Its 64 MB parent array no longer fits in the 16 MB L3 cache, so each union probably costs more and skipping saves more, but I have not profiled this.

On small graphs and on low-degree graphs such as California roads, it is slightly slower than parallel DSU (0.83× to 0.90× in the results). See the [CC results](/cc/results) for the measurements.
