#include <iostream>
#include "graph.hpp"

int main(){
    // input is one formated
    // reading a undirected graph with n vertices, m edged
    // input format: n,m, 
    // m lines: u,v edge from vertice u to v
    int n,m;
    std::cin >> n >> m;
    Graph g(n);
    for (int i = 0; i < m; i++){
        int u, v;
        std::cin >> u >> v;
        u--;
        v--;
        g.add_edge(u, v);
        g.add_edge(v, u);
    }
    g.build_csr();
    auto bfs = g.bfs_sequential(0);

    for (auto& v: bfs) {
        std::cout << v << " ";
    }
    std::cout << "\n";
}

