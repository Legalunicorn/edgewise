#pragma once 
#include <vector>
#include <stdexcept>
#include <cstddef>
#include <span>

// represents a graph by CSR 
// graph can only be build once
struct Graph{
    // graph stored as adjacency list
    std::vector<std::vector<int>> adj_list;
    size_t size;
    explicit Graph(std::size_t n) : adj_list(n), size(n) {}
    
    void add_edge(int u, int v){
        // our design choice -> build graph once only
        if (csr_built) {
            throw std::logic_error("Cannot add edges to graph after CSR has been built");
        }
        adj_list[u].push_back(v);
    }

    void build_csr() {
        if (csr_built) return; 
        std::vector<std::size_t> offsets(size+1, 0);
        for (std::size_t u = 0; u < size; u++){
            offsets[u+1] = offsets[u] + adj_list[u].size();
        }
        std::vector<int> flat;
        flat.reserve(offsets.back()); 
        for (const auto& vs: adj_list){
            flat.insert(flat.end(), vs.begin(), vs.end());
        }

        offsets_.swap(offsets);
        flat_neigbours_.swap(flat);
        // release adj list construction storage
        std::vector<std::vector<int>>{}.swap(adj_list);
        csr_built = true;
    }

    std::span<const int> neighbours(int u) const {
        if (!csr_built) throw std::logic_error("Cannot return neighbours before CSR has been build");
        const auto start = offsets_[u];
        const auto count = offsets_[u+1] - start; 
        return std::span<const int>(flat_neigbours_).subspan(start, count);
    }

    // return the dist array from s 
    // -1: unreachable
    std::vector<int> bfs_sequential(int s) const;

    // V1 - use one shared mutex 
    std::vector<int> bfs_parallel_v1(int s, std::size_t worker_count) const;

    // V2 - Atomic
    std::vector<int> bfs_parallel_v2(int s, std::size_t worker_count) const;

    // V3: persistent workers
    std::vector<int> bfs_parallel_v3(int s, std::size_t worker_count) const ;

private:
    std::vector<std::size_t> offsets_;
    std::vector<int> flat_neigbours_;
    bool csr_built = false;
};
