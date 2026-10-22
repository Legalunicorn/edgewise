#include "generate_graph.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

// bfs is a callable that runs one BFS and returns its distances.
bool benchmark_bfs(const char* implementation, std::size_t workers,
                   std::size_t run, const std::vector<int>& expected, auto bfs,
                   std::size_t n, std::size_t m, std::uint32_t seed) {
    const auto start = std::chrono::steady_clock::now();
    const auto result = bfs(); // Time the complete BFS, including initialization and worker setup.
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double, std::milli> elapsed = end - start;

    if (result != expected) { // Check every measured and warm-up traversal after timing.
        std::cerr << implementation << " returned incorrect distances with "
                  << workers << " workers (vertices=" << n << ", edges=" << m
                  << ", seed=" << seed << ", run=" << run << ")\n";
        return false;
    }

    if (run != 0) { // Run zero is warm-up: check it, but do not record its timing.
        std::cout << implementation << ',' << n << ',' << m << ',' << seed << ',' << workers << ',' << run << ','
                  << elapsed.count() << '\n';
    }
    return true;
}

int benchmark_graph(std::size_t n, std::size_t m, std::uint32_t seed){
    Graph g = gen_graph::random_connected(n, m, seed); // Generate outside timing.
    const auto expected = g.bfs_sequential(0); // Untimed correctness reference.

    const std::vector<std::size_t> worker_counts = {1, 2, 3, 4, 6, 8, 10, 12, 14, 16};

    for (std::size_t run = 0; run <= 5; ++run) { // Run zero is warm-up; record runs 1-5.
        if (!benchmark_bfs("sequential", 1, run, expected, [&] { return g.bfs_sequential(0); }, n, m, seed)) return 1;

        for (std::size_t w : worker_counts) { // Use the same worker counts for every graph.
            if (!benchmark_bfs("mutex_v1", w, run, expected, [&] { return g.bfs_parallel_v1(0, w); }, n, m, seed)) return 1;
            if (!benchmark_bfs("atomic_v2", w, run, expected, [&] { return g.bfs_parallel_v2(0, w); }, n, m, seed)) return 1;
            if (!benchmark_bfs("persistent_v3", w, run, expected, [&] { return g.bfs_parallel_v3(0, w); }, n, m, seed)) return 1;
        }
    }
    return 0; // All traversals matched the reference.
}

int main() {
    std::cout << "implementation,vertices,edges,seed,workers,run,time_ms\n"; // One header for the entire CSV.
    if (benchmark_graph(200'000, 800'000, 67) != 0) return 1; // Stop if a traversal fails.
    if (benchmark_graph(500'000, 2'000'000, 67) != 0) return 1;
    if (benchmark_graph(1'000'000, 4'000'000, 67) != 0) return 1;
    if (benchmark_graph(1'500'000, 6'000'000, 67) != 0) return 1;
    return 0;
}
