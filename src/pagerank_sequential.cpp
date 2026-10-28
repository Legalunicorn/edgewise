#include "graph.hpp"
#include <algorithm>
#include <cmath>

std::vector<double> Graph::pagerank_sequential(double damping,
        double tolerance, std::size_t max_iterations) const{
    double init_score = 1.0 / size;
    std::size_t total_iterations = 0;
    // pagerank score, to return
    std::vector<double> pr(size, init_score);
    std::vector<double> next_pr(size, init_score);
    double max_delta = 1.0, dangling = 0.0, next_dangling = 0.0;
    
    // calculate the initial dangling first
    for (std::size_t u = 0; u < size; u++){
        if (outdegree(u) == 0) dangling += pr[u];
    }

    while(total_iterations < max_iterations && max_delta > tolerance){
        max_delta = 0.0;
        next_dangling = 0.0;
        for (std::size_t u = 0; u < size; u++){
            double score = 0.0;
            for (auto& v: incoming_neighbours(u)) {
                score += pr[v] / outdegree(v);
            }
            next_pr[u] = (1.0-damping) / size + damping * (score + dangling/size);
            if (outdegree(u) == 0) next_dangling += next_pr[u];
            max_delta = std::max(max_delta, std::abs(next_pr[u] - pr[u]));
        }
        total_iterations++;
        swap(pr, next_pr);
        dangling = next_dangling;
    }
    return pr;
}
