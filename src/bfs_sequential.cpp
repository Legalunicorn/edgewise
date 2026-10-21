#include "graph.hpp"
std::vector<int> Graph::bfs_sequential(int s) const{
    std::vector<int> distance(size, -1);
    if (s < 0 || s >= size) {
        std::cerr << "Source s must be >= 0 and < Size of graph";
        return distance;
    }

    std::queue<int> frontier;
    frontier.push(s);
    distance[s] = 0;
    int curr_dist = 0;
    while(frontier.size() > 0) {
        int len = frontier.size();
        curr_dist++;
        for (size_t i = 0; i < len; i++){
            int u = frontier.front();
            frontier.pop();
            for (auto& v: adj_list[u]){
                if (distance[v] != -1) continue;
                frontier.push(v);
                distance[v] = curr_dist;
            }
        }
    }
    return distance;
};

