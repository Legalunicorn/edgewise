#include "graph.hpp"
#include <atomic>
#include <barrier>
#include <thread>

// v3: persistent workers
// the idea is two use two barriers, like a temporary join
// with every iteration of the frontier, we ensure all workers can start
// when a worker is done, the stop barrier is incremeneted 
std::vector<int> Graph::bfs_parallel_v3(int s, std::size_t worker_count) const {
    // starting variables
    std::vector<std::atomic<int>> distance(size);
    std::vector<std::vector<int>> outputs(worker_count);
    for (auto& d: distance) d.store(-1, std::memory_order_relaxed);
    distance[s].store(0);
    std::vector<int> curr{s};
    int curr_depth = 0;
    bool done = false;

    // main thread + workers
    const auto total_active = static_cast<ptrdiff_t>(worker_count +1);
    std::barrier<> start_round(total_active);
    std::barrier<> end_round(total_active);


    // for a single worker thread: scan a range of the frontier
    auto scan = [&](std::size_t start, std::size_t end, std::size_t id) {
        auto& out = outputs[id];
        for (std::size_t i = start; i < end; i++){
            const int u = curr[i];
            for (int v: neighbours(u)){
                if (distance[v].load()  != -1) continue;
                int expected = -1;
                if (distance[v].compare_exchange_strong(expected, curr_depth + 1)){
                    out.push_back(v);
                }
            }
        }
    };


    std::vector<std::jthread> workers;


    auto worker_loop = [&](std::size_t id) -> void{
        while(true){
            // all wait at the frontier
            start_round.arrive_and_wait();
            if (done) break;
            auto& out = outputs[id];
            out.clear();
            const std::size_t begin = curr.size() * id / worker_count;
            const std::size_t end = curr.size() * (id+1) / worker_count;
            scan(begin, end, id);

            end_round.arrive_and_wait();
        }
    };

    // create worker threads, and bring them to the start_round
    for (std::size_t id = 0; id < worker_count; id++){
        workers.emplace_back(worker_loop, id);
    }

    // current bfs layer (frontier)
    while(!curr.empty()){ // process the frontier
        start_round.arrive_and_wait();
        end_round.arrive_and_wait();
        std::vector<int> next;
        for (const auto& out: outputs){ // all nodes to the next frontier
            next.insert(next.end(), out.begin(), out.end());
        }
        curr.swap(next);
        curr_depth++;
    }
    // completed, break all workers out the loop
    done = true;
    start_round.arrive_and_wait();

    // close all the workers
    for (auto& w: workers) w.join();

    std::vector<int> res(size);
    for(std::size_t i = 0; i < size; i++){
        res[i] = distance[i].load();
    }
    return res;
};
