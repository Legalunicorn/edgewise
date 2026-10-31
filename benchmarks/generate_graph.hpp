#pragma once
#include "graph.hpp"
#include <cstddef>
#include <cstdint>

namespace gen_graph{

    // helper methods: generate the graph without building csr 

    // n -> size of binary tree
    Graph binary_tree(std::size_t n);
    Graph binary_tree_helper(std::size_t n);

    // n -> vertices 
    // m -> edges 
    Graph random_connected(
        std::size_t n,
        std::size_t m,
        std::uint32_t seed
    );
    Graph random_connected_helper(
        std::size_t n,
        std::size_t m,
        std::uint32_t seed
    );

    Graph random_undirected(
            std::size_t n,
            std::size_t m,
            std::uint32_t seed
    );
}
