#include "generate_graph.hpp"
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

constexpr double kDamping = 0.85;
constexpr double kTolerance = 1e-10;
constexpr std::size_t kMaxIterations = 100;
constexpr double kScoreTolerance = 1e-10;

// similar to bfs benchmarks
bool benchmark_pagerank(const char* implementation, std::size_t workers,
                        std::size_t run, const std::vector<double>& expected, auto pagerank,
                        std::size_t n, std::size_t m, std::uint32_t seed) {
    const auto start = std::chrono::steady_clock::now();
    const auto result = pagerank(); // Time the complete call, including initialization and workers.
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double, std::milli> elapsed = end - start;

    if (result.size() != expected.size()) {
        std::cerr << implementation << " returned an incorrect score count\n";
        return false;
    }

    double total_score = 0.0;
    for (std::size_t u = 0; u < result.size(); ++u) { // Check scores outside timing.
        if (!std::isfinite(result[u]) || result[u] < 0.0 ||
            std::abs(result[u] - expected[u]) > kScoreTolerance) {
            std::cerr << implementation << " returned an incorrect score at vertex " << u
                      << " with " << workers << " workers (vertices=" << n << ", edges=" << m
                      << ", seed=" << seed << ", run=" << run << ")\n";
            return false;
        }
        total_score += result[u];
    }
    if (std::abs(total_score - 1.0) > 1e-8) {
        std::cerr << implementation << " returned scores that do not sum to 1\n";
        return false;
    }

    if (run != 0) { // Run zero is warm-up: check it, but do not record its timing.
        std::cout << implementation << ',' << n << ',' << m << ',' << seed << ','
                  << workers << ',' << run << ',' << elapsed.count() << ','
                  << kDamping << ',' << kTolerance << ',' << kMaxIterations << '\n';
    }
    return true;
}

int benchmark_graph(std::size_t n, std::size_t m, std::uint32_t seed) {
    Graph g = gen_graph::random_connected(n, m, seed); // Generate and build CSR outside timing.
    const auto expected = g.pagerank_sequential(kDamping, kTolerance, kMaxIterations);

    const std::vector<std::size_t> worker_counts = {1, 2, 3, 4, 6, 8, 10, 12, 14, 16};

    for (std::size_t run = 0; run <= 5; ++run) { // One warm-up sweep, then five measured sweeps.
        if (!benchmark_pagerank("sequential", 1, run, expected, [&] {
            return g.pagerank_sequential(kDamping, kTolerance, kMaxIterations);
        }, n, m, seed)) return 1;

        for (std::size_t w : worker_counts) {
            if (!benchmark_pagerank("pagerank_parallel", w, run, expected, [&] {
                return g.pagerank_parallel(w, kDamping, kTolerance, kMaxIterations);
            }, n, m, seed)) return 1;
        }
    }
    return 0;
}

int main() {
    std::cout << "implementation,vertices,edges,seed,workers,run,time_ms,damping,tolerance,max_iterations\n";
    if (benchmark_graph(200'000, 800'000, 67) != 0) return 1; // m counts undirected edges.
    if (benchmark_graph(500'000, 2'000'000, 67) != 0) return 1;
    if (benchmark_graph(1'000'000, 4'000'000, 67) != 0) return 1;
    if (benchmark_graph(1'500'000, 6'000'000, 67) != 0) return 1;
    return 0;
}
