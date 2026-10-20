#pragma once 
#include <queue>
#include <vector>
#include <iostream>
#include <thread>
#include <cstddef>
#include <atomic>
#include <mutex>
#include <barrier>

// represents a graph 
// responsibile for all graph functions
struct Graph{
    // graph stored as adjacency list
    std::vector<std::vector<int>> adj_list;
    std::vector<bool> seen;
    size_t size;
    Graph(size_t n){
        adj_list.resize(n);
        seen.resize(n);
        size = n;
    }

    // add a direced edged from u to v
    // 1-indexed
    void add_edge(int u, int v){
        adj_list[u].push_back(v);
    }

    // return the dist array from s 
    // -1: unreachable
    std::vector<int> bfs_sequential(int s){
        std::vector<int> distance(size, -1);
        if (s < 0 || s >= size) {
            std::cerr << "Source s must be >= 0 and < Size of graph";
            return distance;
        }

        seen.assign(size, false);
        std::queue<int> frontier;
        frontier.push(s);
        distance[s] = 0;
        seen[s] = true;
        int curr_dist = 0;
        while(frontier.size() > 0) {
            int len = frontier.size();
            curr_dist++;
            for (size_t i = 0; i < len; i++){
                int u = frontier.front();
                frontier.pop();
                for (auto& v: adj_list[u]){
                    if (seen[v]) continue;
                    seen[v] = true;
                    frontier.push(v);
                    distance[v] = curr_dist;
                }
            }
        }
        return distance;
    };

    // parallel version 1
    // split the frontier among worker threads
    // there is still heavy sequentual bottle neck when visiting neighbours
    std::vector<int> bfs_parallel_v1(int s, std::size_t worker_count) const {
        // local discovery state shared among worker threads
        std::vector<int> distance(adj_list.size(), -1);
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
                    for (int v: adj_list[u]){
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

    // V2 - Atomic
    // split the frontier among worker threads 
    // there is still heavy sequentual bottle neck when visiting neighbours
    std::vector<int> bfs_parallel_v2(int s, std::size_t worker_count) const {
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
                    for (int v: adj_list[u]){
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

    // v3: persistent workers
    // the idea is two use two barriers, like a temporary join
    // with every iteration of the frontier, we ensure all workers can start
    // when a worker is done, the stop barrier is incremeneted 
    //
    std::vector<int> bfs_parallel_v3(int s, std::size_t worker_count) const {
        // starting variables
        std::vector<std::atomic<int>> distance(size);
        std::vector<std::vector<int>> outputs(worker_count);
        for (auto& d: distance) d.store(-1);
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
                for (int v: adj_list[u]){
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
};
