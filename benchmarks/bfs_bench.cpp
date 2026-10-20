#include "generate_graph.hpp"
#include <chrono>
#include <cstddef>
#include <iostream>
#include <vector>

// bfs is a callable that runs one BFS and returns its distances.
bool benchmark_bfs(const char* implementation, std::size_t workers,
                   std::size_t run, const std::vector<int>& expected, auto bfs) {
    const auto start = std::chrono::steady_clock::now(); // Start wall-clock timing.
    const auto result = bfs(); // Time the complete BFS call, including worker setup.
    const auto end = std::chrono::steady_clock::now(); // Stop before checking or printing.
    const std::chrono::duration<double, std::milli> elapsed = end - start;

    if (result != expected) { // Check every measured and warm-up traversal.
        std::cerr << implementation << " returned incorrect distances with "
                  << workers << " workers\n";
        return false;
    }

    if (run != 0) { // Run zero is warm-up: check it, but do not record its timing.
        std::cout << implementation << ',' << workers << ',' << run << ','
                  << elapsed.count() << '\n';
    }
    return true;
}

int main() {
    Graph g = gen_graph::random_connected(200'000, 800'000, 67); // Generate outside timing.
    const auto expected = g.bfs_sequential(0); // Untimed correctness reference.
    std::cout << "implementation,workers,run,time_ms\n";

    for (std::size_t run = 0; run <= 5; ++run) { // Run zero is warm-up; record runs 1-5.
        if (!benchmark_bfs("sequential", 1, run, expected, [&] { return g.bfs_sequential(0); })) return 1;

        for (std::size_t w = 1; w <= 16; ++w) { // Same worker sweep as before.
            if (!benchmark_bfs("mutex_v1", w, run, expected, [&] { return g.bfs_parallel_v1(0, w); })) return 1;
            if (!benchmark_bfs("atomic_v2", w, run, expected, [&] { return g.bfs_parallel_v2(0, w); })) return 1;
            if (!benchmark_bfs("persistent_v3", w, run, expected, [&] { return g.bfs_parallel_v3(0, w); })) return 1;
        }
    }
}
