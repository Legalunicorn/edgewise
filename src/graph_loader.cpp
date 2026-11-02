#include "graph_loader.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <stdexcept>
#include <utility>
#include <sstream>
#include <map>
#include <set>

// reads a graph using File IO, ignore comments of the file
// undirected = true if the graph is undirected
Graph load_edge_list(const std::string& path, bool undirected) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("cannot open the file: " + path);
    }

    std::string line;
    // std::vector<std::pair<int,int>> edgelist;
    std::set<std::pair<int,int>> edge_set;

    // coordinate compression on the input
    int id = 0;
    std::map<int,int> compress;
    std::vector<std::pair<int,int>> edge_list;


    int u, v;
    while(std::getline(file,line)){
        // comments start with "#"
        std::istringstream row(line);
        // discard white space
        row >> std::ws;
        // skip blank lines, comments, indented commments
        if (row.eof() || row.peek()=='#') continue;
        row >> u >> v;
        if (row.fail() || u < 0 || v < 0) throw std::runtime_error("Invalid edge line:  " + line);
        std::string extra;
        row >> extra;
        if (!row.fail()) throw std::runtime_error("Invalid edge line: "+ line);

        // if undirected then order them st u < v
        if (undirected && u > v) std::swap(u,v);
        // ignore duplicate edges in input
        if (edge_set.count({u,v})) continue;

        edge_set.insert({u,v});
    }

    for (auto& [u,v]: edge_set){
        if (!compress.count(u)) compress[u] = id++;
        if (!compress.count(v)) compress[v] = id++;
        edge_list.push_back({compress[u], compress[v]});
    }


    Graph g(id);
    for (auto& [u,v]: edge_list){
        g.add_edge(u, v);
        if (undirected && u!=v) g.add_edge(v, u);
    }
    g.build_csr();
    return g;
}
