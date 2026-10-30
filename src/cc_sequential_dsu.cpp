#include "graph.hpp"
#include <vector>
#include "dsu_sequential.hpp"

std::vector<int> Graph::cc_sequential_dsu() const{
    SequentialDSU dsu(size);
    for (int u = 0; u < size; u++){
        for (auto v: neighbours(u)) {
            dsu.union_set(u, v);
        }
    }
    std::vector<int> component(size);
    for (int u = 0; u < size; u++) component[u] = dsu.find(u);
    return component;
};
