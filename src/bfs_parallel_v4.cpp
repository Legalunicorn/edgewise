#include "graph.hpp"
#include <vector>
#include <thread>
#include <atomic> 
#include <barrier> 
#include <algorithm>

std::vector<int> Graph::bfs_parallel_v4(int s, std::size_t worker_count) const {
    // starting variables
    std::vector<std::atomic<int>> distance(size);
    std::vector<std::vector<int>> outputs(worker_count);
    for (auto& d: distance) d.store(-1, std::memory_order_relaxed);
    distance[s].store(0);

    std::vector<int> curr{s};
    std::vector<int> next;
    // for workers to discovery which part of next to copy into
    std::vector<std::size_t> copy_offsets_(worker_count + 1);

    int curr_depth = 0;
    bool done = false;
    std::size_t curr_count = 1; 

    // main thread + workers
    const auto total_active = static_cast<ptrdiff_t>(worker_count +1);
    std::barrier<> start_round(total_active);
    std::barrier<> end_round(total_active);
    std::vector<std::jthread> workers;


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


    // worker looop that lives through out the algorithm
    // repeatedly scan a frontier, wait, scans a new fronter 
    auto worker_loop = [&](std::size_t id) -> void{
        while(true){
            // ## ROUND 1: scan bfs
            // worker wait at the frontier
            start_round.arrive_and_wait();
            // released  when all has arrived
            if (done) break;
            auto& out = outputs[id];
            out.clear();
            const std::size_t begin = curr_count * id / worker_count;
            const std::size_t end = curr_count * (id+1) / worker_count;
            scan(begin, end, id);

            // contribute offset size
            copy_offsets_[id+1] = outputs[id].size();
            // worker waits and the end barrier
            end_round.arrive_and_wait();

            // ## ROUND 2: write to next
            start_round.arrive_and_wait();
            const std::size_t dest = copy_offsets_[id];
            std::copy(out.begin(), out.end(), next.begin() + dest);
            // for (std::size_t i = 0; i < out.size(); i++){
            //     next[dest + i] = out[i];
            // }
            end_round.arrive_and_wait();
        }
    };

    // create worker threads, and bring them to the start_round
    for (std::size_t id = 0; id < worker_count; id++){
        workers.emplace_back(worker_loop, id);
    }

    // main thread: coordinator
    // current bfs layer (frontier)
    while(curr_count != 0){ // process the frontier
        // #### Start round 1
        start_round.arrive_and_wait();
        end_round.arrive_and_wait();

        // buffer between end round 1, and start of round 2
        // precompute the prefix sum first, while all worker thread swait to start round 2
        for (std::size_t i = 1; i <= worker_count; i++) copy_offsets_[i] += copy_offsets_[i-1];
        const std::size_t next_count = copy_offsets_.back();
        curr_count = next_count;
        if (next.size() < next_count) next.resize(copy_offsets_.back()); // grow when necesary
        
        // #### Start round 2
        start_round.arrive_and_wait(); // all workers do their copying
        end_round.arrive_and_wait();  // coordinator close the second round 

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


