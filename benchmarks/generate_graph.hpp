#pragma once
#include "graph.hpp"
#include <cstddef>
#include <cstdint>

namespace gen_graph{

    // n -> size of binary tree
    Graph binary_tree(std::size_t n);

    // n -> vertices 
    // m -> edges 
    Graph random_connected(
        std::size_t n,
        std::size_t m,
        std::uint32_t seed
    );

}
