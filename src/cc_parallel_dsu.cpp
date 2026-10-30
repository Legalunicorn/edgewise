#include "graph.hpp"
#include <vector>
#include "dsu_concurrent.hpp"
#include <thread>

std::vector<int> Graph::cc_parallel_dsu(std::size_t worker_count) const{
    ConcurrentDSU dsu(size);
    std::vector<int> res(size);
    std::vector<std::jthread> workers;

    auto work = [&](std::size_t start, std::size_t end, std::size_t id) -> void {
        for (int u = start ; u < end; u++){
            for (auto& v: neighbours(u)){
                dsu.union_set(u, v);
            }
        }
    };

    //main thread
    for (std::size_t w = 0; w < worker_count; w++){
        std::size_t start = size*w/worker_count;
        std::size_t end = size*(w+1)/worker_count;
        workers.emplace_back(work, start, end, w);
    }

    for (auto& w: workers) w.join();
    for (std::size_t u = 0; u < size; u++) res[u] = dsu.find(u);
    return res;
};
