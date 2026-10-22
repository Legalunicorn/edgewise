#include "graph.hpp"
#include <atomic>
#include <thread>

// V2 - Atomic
// split the frontier among worker threads 
// there is still heavy sequentual bottle neck when visiting neighbours
std::vector<int> Graph::bfs_parallel_v2(int s, std::size_t worker_count) const {
    std::vector<std::atomic<int>> distance(size);
    for (auto& d: distance) d.store(-1);
    distance[s].store(0);
    // current bfs layer (frontier)
    std::vector<int> curr({s});
    int curr_depth = 0;
    while(!curr.empty()){ // process the frontier
                          // each worker outputs to its own vector
        std::vector<std::vector<int>> eval(worker_count);

        // work function for each worker 
        auto scan = [&](std::size_t start, std::size_t end, std::size_t id) {
            auto& out = eval[id];
            for (std::size_t i = start; i < end; i++){
                const int u = curr[i];
                for (int v: neighbours(u)){
                    int expected = -1;
                    if (distance[v].compare_exchange_strong(expected, curr_depth + 1)){
                        out.push_back(v);
                    }
                }
            }
        };

        std::vector<std::jthread> workers;
        for (std::size_t id = 0; id < worker_count; id++){
            // consider abstracting this logic
            // total work / worker count * (id)  --> total work 
            const std::size_t begin = curr.size() * id / worker_count;
            const std::size_t end = curr.size() * (id+1) / worker_count;

            // constructs a jthread, and executes scan(begin, end, id)
            workers.emplace_back(scan, begin, end, id);
        }

        // combine the results
        for (auto& w: workers) w.join();
        std::vector<int> next;
        for (const auto& e: eval){
            next.insert(next.end(), e.begin(), e.end());
        }
        // prepare the next frontier
        curr.swap(next);
        curr_depth++;
    }
    std::vector<int> res(size);
    for(std::size_t i = 0; i < size; i++){
        res[i] = distance[i].load();
    }
    return res;
}
