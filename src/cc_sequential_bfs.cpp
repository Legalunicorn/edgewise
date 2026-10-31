#include "graph.hpp"
#include <queue>


std::vector<int> Graph::cc_sequential_bfs() const{
    std::vector<int> component(size, -1);
    int curr_id = 0;
    for (int u = 0; u < size; u++){
        if (component[u] != -1) continue; // already connected
        std::queue<int> frontier;
        frontier.push(u);
        component[u] = curr_id;
        while(frontier.size() > 0) {
            int u = frontier.front();
            frontier.pop();

            for (auto& v: neighbours(u)){
                if (component[v] != -1) continue;
                frontier.push(v);
                component[v] = curr_id;
            }
        }
        curr_id++;
    }
    return component;
};

