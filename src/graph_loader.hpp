#pragma once
#include "graph.hpp"
#include <string>


// reads a graph using File IO, ignore comments of the file
// undirected = true if the graph is undirected
Graph load_edge_list(const std::string& path, bool undirected);
