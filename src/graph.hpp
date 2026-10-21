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
    std::vector<int> bfs_sequential(int s) const;

    // std::vector<int> bfs_sequential(int s){
    //     std::vector<int> distance(size, -1);
    //     if (s < 0 || s >= size) {
    //         std::cerr << "Source s must be >= 0 and < Size of graph";
    //         return distance;
    //     }
    //
    //     seen.assign(size, false);
    //     std::queue<int> frontier;
    //     frontier.push(s);
    //     distance[s] = 0;
    //     seen[s] = true;
    //     int curr_dist = 0;
    //     while(frontier.size() > 0) {
    //         int len = frontier.size();
    //         curr_dist++;
    //         for (size_t i = 0; i < len; i++){
    //             int u = frontier.front();
    //             frontier.pop();
    //             for (auto& v: adj_list[u]){
    //                 if (seen[v]) continue;
    //                 seen[v] = true;
    //                 frontier.push(v);
    //                 distance[v] = curr_dist;
    //             }
    //         }
    //     }
    //     return distance;
    // };

    // V1 - use one shared mutex 
    // has huge bottle next for visiting neihgbours
    // as only one neighbour can be visited at a time
    std::vector<int> bfs_parallel_v1(int s, std::size_t worker_count) const;

    // V2 - Atomic
    // split the frontier among worker threads 
    // there is still heavy sequentual bottle neck when visiting neighbours
    std::vector<int> bfs_parallel_v2(int s, std::size_t worker_count) const;

    // v3: persistent workers
    // the idea is two use two barriers, like a temporary join
    // with every iteration of the frontier, we ensure all workers can start
    // when a worker is done, the stop barrier is incremeneted 
    std::vector<int> bfs_parallel_v3(int s, std::size_t worker_count) const ;
};
