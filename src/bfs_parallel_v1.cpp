#include "graph.hpp"
#include <vector>
#include <cstddef>
#include <mutex>
#include <thread>


// parallel version 1
// split the frontier among worker threads
// there is still heavy sequentual bottle neck when visiting neighbours
// local discovery state shared among worker threads
std::vector<int> Graph::bfs_parallel_v1(int s, std::size_t worker_count) const {
    std::vector<int> distance(size, -1);
    distance[s] = 0;
    // current bfs layer (frontier)
    std::vector<int> curr({s});
    int curr_depth = 0;
    // single shared mutex accorss workers
    std::mutex discovery_mutex;
    while(!curr.empty()){ // process the frontier
                          // each worker outputs to its own vector
        std::vector<std::vector<int>> eval(worker_count);

        // work function for each worker
        auto scan = [&](std::size_t start, std::size_t end, std::size_t id) {
            auto& out = eval[id];
            for (std::size_t i = start; i < end; i++){
                const int u = curr[i];
                for (int v: neighbours(u)){
                    // discovery, to ensure only 1 worker can own the current "v"
                    bool discovered = false;
                    // create block for the lock guard, with auto destruction when the block ends
                    {
                        std::lock_guard<std::mutex> guard(discovery_mutex);
                        if (distance[v] == -1){
                            distance[v] = curr_depth + 1;
                            // this work has found discovery, it "owned" this discovered
                            discovered = true;
                        }
                    }
                    if (discovered) out.push_back(v);
                }
            }
        };

        std::vector<std::jthread> workers;
        for (std::size_t id = 0; id < worker_count; id++){
            // consider abstracting this logic
            // total work / worker count * (id)  --> totka work
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
    return distance;
}



