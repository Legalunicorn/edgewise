#include "generate_graph.hpp"
#include <random>
#include <stdexcept>
#include <utility>
#include <set>

namespace gen_graph{

    void check_valid_graph_size(std::size_t n){
        if (n == 0) throw std::invalid_argument("Graph size cannot be 0");
        if (n > (1<<30)) throw std::invalid_argument("Graph size must be less than 1<<30");
    }


    Graph binary_tree(std::size_t n){
        Graph g = binary_tree_helper(n);
        g.build_csr();
        return g;
    }
    Graph binary_tree_helper(std::size_t n){
        check_valid_graph_size(n);
        Graph g(n);
        // 0 -> 1,2 
        // 1 -> 3,4 
        // 2 -> 5,6
        // formula for parent is (i-1)/2
        // each vertex has one parent exactly
        for (std::size_t i = 1; i < n; i++){
            const int u = static_cast<int>(i);
            const int v = static_cast<int>((i-1)/2);
            g.add_edge(u, v);
            g.add_edge(v, u);
        }
        return g;
    }


    Graph random_connected(std::size_t n, std::size_t m, std::uint32_t seed) {
        Graph g = random_connected_helper(n, m, seed);
        g.build_csr();
        return g;
    }
    // 1. construct a random spanning tree using n-1 edges
    Graph random_connected_helper(std::size_t n, std::size_t m, std::uint32_t seed){
        check_valid_graph_size(n);
        // max edges is n choose 2 (undirected edges)
        const auto max_edges = static_cast<std::uint64_t>(n*(n-1)/2);
        if (m < n-1 || m > max_edges ) throw std::invalid_argument("Invalid edge count. n-1 <= m <= n choose 2");
        Graph g = binary_tree_helper(n);
        std::size_t total_edges = n-1;
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> pick(0, n-1);
        while(total_edges < m) {
            const int u = pick(rng);
            const int v = pick(rng);
            // no selft loops
            if (u == v) continue;
            // check if edge already exist
            bool dup = false;
            for (auto& neighbour: g.adj_list[u]){
                if (neighbour==v){
                    dup = true;
                    break;
                }
            }
            if (dup) continue;
            g.add_edge(u, v);
            g.add_edge(v, u);
            total_edges++;
        }
        return g;
    }

    Graph random_undirected(std::size_t n, std::size_t m, std::uint32_t seed) {
        check_valid_graph_size(n);
        const auto max_edges = static_cast<std::uint64_t>(n*(n-1)/2);
        if (m > max_edges ) throw std::invalid_argument("Edge count cannot exceed n choose 2 ");

        Graph g(n);
        std::size_t total_edges = 0;
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> pick(0, n-1);
        std::set<std::pair<int,int>> st;
        while(total_edges < m) {
            int u = pick(rng);
            int v = pick(rng);
            // no selft loops
            if (u == v) continue;
            if (u > v) std::swap(u,v);
            if (st.count({u,v})) continue;
            st.insert({u,v});
            g.add_edge(u, v);
            g.add_edge(v, u);
            total_edges++;
        }
        g.build_csr();
        return g;
    }


}
