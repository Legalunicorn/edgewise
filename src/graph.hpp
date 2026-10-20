#pragma once 
#include <queue>
#include <vector>
#include <iostream>
#include <thread>
#include <cstddef>
#include <atomic>

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
    std::vector<int> bfs_parallel(int s, std::size_t worker_count) const {
        // local discovery state shared among worker threads
        // std::vector<int> distance(adj_list.size(), -1);
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
};
