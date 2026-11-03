#include "generate_graph.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <vector>
#include <unordered_map>
#include "graph_loader.hpp"


bool expect_same_components(const std::vector<int>& actual, const std::vector<int>& expected) {
    if(actual.size() != expected.size()) return false;
    // we need a 1-1 mapping
    std::unordered_map<int, int> front;
    std::unordered_map<int, int> back;
    for (std::size_t i = 0; i < actual.size(); ++i) {
        const auto forward = front.try_emplace(actual[i], expected[i]).first;
        if (forward->second != expected[i]) return false;
        const auto reverse = back.try_emplace(expected[i], actual[i]).first;
        if (reverse->second != actual[i]) return false;
    }
    return true;
}


// cc is a callable that runs one cc and returns the components
bool benchmark_cc(const char* implementation, std::size_t workers,
                   std::size_t run, const std::vector<int>& expected, auto cc,
                   std::size_t n, std::size_t m, std::int64_t seed) {
    const auto start = std::chrono::steady_clock::now();
    const auto result = cc(); // Time the complete CC, including initialization and worker setup.
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double, std::milli> elapsed = end - start;

    if (!expect_same_components(result, expected)) { // Check every measured and warm-up traversal after timing.
        std::cerr << implementation << " returned incorrect components with "
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

int benchmark_graph(const Graph& g, std::size_t m, std::int64_t seed){
    const std::size_t n = g.size;
    const auto expected = g.cc_sequential_bfs(); // Untimed correctness ref

    const std::vector<std::size_t> worker_counts = {1, 2, 3, 4, 6, 8, 10, 12, 14, 16};

    for (std::size_t run = 0; run <= 5; ++run) { // Run zero is warm-up; record runs 1-5.
        if (!benchmark_cc("cc_sequential_bfs", 1, run, expected, [&] { return g.cc_sequential_bfs(); }, n, m, seed)) return 1;
        if (!benchmark_cc("cc_sequential_dsu", 1, run, expected, [&] { return g.cc_sequential_dsu(); }, n, m, seed)) return 1;

        for (std::size_t w : worker_counts) { // Use the same worker counts for every graph.
            if (!benchmark_cc("cc_parallel_dsu", w, run, expected, [&] { return g.cc_parallel_dsu(w); }, n, m, seed)) return 1;
            if (!benchmark_cc("cc_parallel_afforest", w, run, expected, [&] { return g.cc_parallel_afforest(w); }, n, m, seed)) return 1;
        }
    }
    return 0; // All traversals matched the reference.
}

int benchmark_generated_graph(std::size_t n, std::size_t m, std::uint32_t seed){
    const Graph g = gen_graph::random_undirected(n,m,seed);
    return benchmark_graph(g, m, seed);
}


int main(int argc, char* argv[]) {

    if (argc > 2) {
        std::cerr << "[usage] cc_bench FILENAME\n";
        return 1;
    }
    std::cout << "implementation,vertices,edges,seed,workers,run,time_ms\n"; // One header for the entire CSV.
    bool isUndirected = true; // connected component
    if (argc == 2){
        const Graph g = load_edge_list(argv[1], isUndirected);
        // count the edges
        // TODO: count it while building the graph
        std::size_t m = 0;
        for (int u = 0; u < g.size; u++){
            for (auto& v: g.neighbours(u)){
                if (u <= v) m++;
            }
        }
        return benchmark_graph(g, m, -1); // no seed for non random generated
    }
    if (benchmark_generated_graph(200'000, 800'000, 67) != 0) return 1; // Stop if a traversal fails.
    if (benchmark_generated_graph(500'000, 2'000'000, 67) != 0) return 1;
    if (benchmark_generated_graph(1'000'000, 4'000'000, 67) != 0) return 1;
    if (benchmark_generated_graph(1'500'000, 6'000'000, 67) != 0) return 1;
    return 0;
}
