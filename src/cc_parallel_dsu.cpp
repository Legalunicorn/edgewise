#include "graph.hpp"
#include "dsu_concurrent.hpp"
#include <algorithm>
#include <atomic>
#include <barrier>
#include <cstddef>
#include <thread>
#include <vector>

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
