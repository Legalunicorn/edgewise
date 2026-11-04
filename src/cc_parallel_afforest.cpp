#include "graph.hpp"
#include <stdexcept>
#include <vector>
#include "dsu_concurrent.hpp"
#include <thread>
#include <barrier>
#include <random>
#include <unordered_map>
#include <atomic>
#include <algorithm>


namespace{


    // #### helper methods 
    // link_round(const g, dsu, r (round), start, end)
    // compress(dsu, start, end);
    // sample_frequent_label(dsu, n);
    // link_remaining(g, dsu, c (id of majority componet), start, end)
    //
    // ### WORKER WORK
    // link(0)  -> anw
    // compress -> anw
    // link(1) -> anw
    // commpress -> anw
    // (nothing - main samples) -> anw
    // link_remaining -> anw
    //
    //
    // ### main thread
    // 1. w workers created
    // 2. ANW x4
    // 3. find C 
    // 4 ANW  (start last)
    // 5. ANW (end last)
    // 6. join, -> return res

    int sample_random(ConcurrentDSU& dsu, std::uint32_t seed, std::size_t n) {
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> pick(0,n-1);
        std::unordered_map<int,int> mp;
        int samples = 1024;
        for (int i = 0; i < samples; i++){
            const int v = pick(rng);
            int c = dsu.find(v);
            mp[c]++;
        }
        int c = 0;
        int best = 0;
        for (const auto& [id, freq]: mp){
            if (freq > best) {
                best = freq;
                c = id;
            }
        }
        return c;
    }

    void link_round(const Graph& g, ConcurrentDSU& dsu, std::size_t r, std::size_t start, std::size_t end){
        for (std::size_t u = start;  u < end ; u++){
            if (g.outdegree(u) <= r) continue;
            const int v = g.neighbours(u)[r];
            dsu.union_set(u, v);
        }
    };

    void compress(ConcurrentDSU& dsu, std::size_t start, std::size_t end){
        for (std::size_t u = start;  u < end ; u++){
            dsu.find(u);
        }
    }

    void fill_results(std::vector<int>& res, ConcurrentDSU& dsu, std::size_t start, std::size_t end){
        for (std::size_t u = start;  u < end ; u++){
            res[u] = dsu.find(u);
        }
    }


    void link_remaining(const Graph& g, ConcurrentDSU& dsu, std::size_t start, std::size_t end, int major_component){
        const int NEIGHBOUR_ROUDNS = 2;
        for (std::size_t u = start;  u < end ; u++){
            if (g.outdegree(u) <= NEIGHBOUR_ROUDNS || dsu.parent[u].load()==major_component) continue;
            for (const auto& v: g.neighbours(u).subspan(NEIGHBOUR_ROUDNS)) {
                dsu.union_set(u, v);
            }
        }
    };
}


std::vector<int> Graph::cc_parallel_afforest(std::size_t worker_count) const{
    if (size == 0) return {};
    if (worker_count == 0) {
        throw std::invalid_argument("Worker count must be positive");
    }
    ConcurrentDSU dsu(size);
    // include the main thread
    std::barrier<> sync(worker_count + 1);
    std::vector<std::jthread> workers;
    workers.reserve(worker_count);

    // during the last stage we split to chunks
    const std::size_t CHUNK = 4096;
    std::atomic<std::size_t> next_chunk{0};

    // is this slow? O(V) initializable??
    std::vector<int> components(size);

    int major_component = 0;

    auto worker_tasks = [&](std::size_t id) -> void {
        // ### WORKER WORK
        // link(0)  -> anw
        // compress -> anw
        // link(1) -> anw
        // commpress -> anw
        // (nothing - main samples) -> anw
        // link_remaining -> anw
        std::size_t start = size*id / worker_count;
        std::size_t end = size*(id+1) / worker_count;

        // round 1 -> process first vertice -> compres
        link_round(*this, dsu, 0, start, end);
        sync.arrive_and_wait(); // 1
        compress(dsu, start, end);
        sync.arrive_and_wait(); // 2 

        // round 2 -> seoncd neighbour
        link_round(*this, dsu, 1, start, end);
        sync.arrive_and_wait(); // 3
        compress(dsu, start, end);
        sync.arrive_and_wait(); // 4

        // (main thread random samples)

        // link remaining by CHUNKS
        sync.arrive_and_wait(); // 5
        while(true){
            const std::size_t s = next_chunk.fetch_add(CHUNK);
            if (s >= size) break;
            std::size_t e = std::min(s + CHUNK, size);
            link_remaining(*this, dsu, s, e, major_component);
        }

        // combine results 
        sync.arrive_and_wait(); // 6
        fill_results(components, dsu, start, end);
    };

    // ### main thread
    // 1. w workers created
    // 2. ANW x4
    // 3. find C 
    // 4 ANW  (start last)
    // 5. ANW (end last)
    // 6. join, -> return res
    for (std::size_t w = 0; w < worker_count; w++){
        workers.emplace_back(worker_tasks, w);
    }
    
    sync.arrive_and_wait(); // 1
    sync.arrive_and_wait(); // 2
    sync.arrive_and_wait(); // 3
    sync.arrive_and_wait(); // 4

    major_component = sample_random(dsu, 67, size);

    sync.arrive_and_wait(); // 5
    sync.arrive_and_wait(); // 6
    for (auto& w: workers) w.join();
    return components;
};




