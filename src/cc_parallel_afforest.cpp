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
