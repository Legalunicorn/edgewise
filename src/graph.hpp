#pragma once 
#include <vector>
#include <stdexcept>
#include <cstddef>
#include <span>

// represents a graph by CSR 
// graph can only be build once
struct Graph{
    // graph stored as adjacency list
    std::vector<std::vector<int>> adj_list, reverse_adj_list;
    size_t size;
    explicit Graph(std::size_t n) : adj_list(n), reverse_adj_list(n), size(n) {}
    
    void add_edge(int u, int v){
        // our design choice -> build graph once only
        if (csr_built) {
            throw std::logic_error("Cannot add edges to graph after CSR has been built");
        }
        adj_list[u].push_back(v);
        reverse_adj_list[v].push_back(u);
    }

    // build CSR for normal graph and inverse graph
    void build_csr() {
        if (csr_built) return; 
        std::vector<std::size_t> offsets(size+1, 0), reverse_offsets(size+1, 0);
        for (std::size_t u = 0; u < size; u++){
            offsets[u+1] = offsets[u] + adj_list[u].size();
            reverse_offsets[u+1] = reverse_offsets[u] + reverse_adj_list[u].size();
        }
        std::vector<int> flat, reverse_flat;
        flat.reserve(offsets.back()); 
        reverse_flat.reserve(reverse_offsets.back()); 
        for (const auto& vs: adj_list){
            flat.insert(flat.end(), vs.begin(), vs.end());
        }
        for (const auto& vs: reverse_adj_list){
            reverse_flat.insert(reverse_flat.end(), vs.begin(), vs.end());
        }

        offsets_.swap(offsets);
        reverse_offsets_.swap(reverse_offsets);
        flat_neigbours_.swap(flat);
        reverse_flat_neigbours_.swap(reverse_flat);
        // release adj list construction storage
        std::vector<std::vector<int>>{}.swap(adj_list);
        std::vector<std::vector<int>>{}.swap(reverse_adj_list);
        csr_built = true;
    }

    std::span<const int> neighbours(int u) const {
        if (!csr_built) throw std::logic_error("Cannot return neighbours before CSR has been build");
        const auto start = offsets_[u];
        const auto count = offsets_[u+1] - start; 
        return std::span<const int>(flat_neigbours_).subspan(start, count);
    }

    std::span<const int> incoming_neighbours(int u) const {
        if (!csr_built) throw std::logic_error("Cannot return indegree before CSR has been build");
        const auto start = reverse_offsets_[u];
        const auto count = reverse_offsets_[u+1] - start; 
        return std::span<const int>(reverse_flat_neigbours_).subspan(start, count);
    }

    std::size_t outdegree(int u) const {
        const auto start = offsets_[u];
        return offsets_[u+1] - start; 
    }

    std::size_t indegree(int u) const {
        const auto start = reverse_offsets_[u];
        return reverse_offsets_[u+1] - start; 

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

    // V4: parallel assembly
    std::vector<int> bfs_parallel_v4(int s, std::size_t worker_count) const;


    // ## Page rank algos
    std::vector<double> pagerank_sequential(double damping = 0.85,
            double tolerance = 1e-6,
            std::size_t max_iterations = 100) const;

    std::vector<double> pagerank_parallel(std::size_t worker_count,
            double damping = 0.85,
            double tolerance = 1e-6,
            std::size_t max_iterations = 100) const;

    std::vector<int> cc_sequential_bfs() const;
    std::vector<int> cc_sequential_dsu() const;
    std::vector<int> cc_parallel_dsu(std::size_t worker_count) const;
    std::vector<int> cc_parallel_afforest(std::size_t worker_count) const;

private:
    std::vector<std::size_t> offsets_;
    std::vector<std::size_t> reverse_offsets_;

    std::vector<int> flat_neigbours_;
    std::vector<int> reverse_flat_neigbours_;
    bool csr_built = false;
};
