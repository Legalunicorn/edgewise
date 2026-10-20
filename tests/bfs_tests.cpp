#include "graph.hpp"
#include <vector>
#include <gtest/gtest.h>


TEST(BFS, DisconnectedVertices) {
    Graph g(3);
    g.add_edge(0,1);
    g.add_edge(1,0);
    const std::vector<int> expected({0,1,-1});
    EXPECT_EQ(expected, g.bfs_sequential(0));
    EXPECT_EQ(expected, g.bfs_parallel(0,1));
    EXPECT_EQ(expected, g.bfs_parallel(0,2));
    EXPECT_EQ(expected, g.bfs_parallel(0,3));
    EXPECT_EQ(expected, g.bfs_parallel_v3(0,1));
    EXPECT_EQ(expected, g.bfs_parallel_v3(0,2));
    EXPECT_EQ(expected, g.bfs_parallel_v3(0,3));
}

TEST(BFS, DiamondGraph){
    Graph g(4);
    g.add_edge(0,1);
    g.add_edge(0,2);
    g.add_edge(1,3);
    g.add_edge(2,3);
    const std::vector<int> expected({0,1,1,2});
    const auto result = g.bfs_sequential(0);
    EXPECT_EQ(expected, result);
    EXPECT_EQ(expected, g.bfs_parallel(0,1));
    EXPECT_EQ(expected, g.bfs_parallel(0,2));
    EXPECT_EQ(expected, g.bfs_parallel(0,3));
    EXPECT_EQ(expected, g.bfs_parallel_v3(0,1));
    EXPECT_EQ(expected, g.bfs_parallel_v3(0,2));
    EXPECT_EQ(expected, g.bfs_parallel_v3(0,3));
}

TEST(BFS, Cycle) {
    Graph g(4);
    g.add_edge(0,1);
    g.add_edge(1,2);
    g.add_edge(2,3);
    g.add_edge(3,0);
    const std::vector<int> expected({0,1,2,3});
    const auto result = g.bfs_sequential(0);
    EXPECT_EQ(expected, result);
    EXPECT_EQ(expected, g.bfs_parallel(0,1));
    EXPECT_EQ(expected, g.bfs_parallel(0,2));
    EXPECT_EQ(expected, g.bfs_parallel(0,3));
    EXPECT_EQ(expected, g.bfs_parallel_v3(0,1));
    EXPECT_EQ(expected, g.bfs_parallel_v3(0,2));
    EXPECT_EQ(expected, g.bfs_parallel_v3(0,3));
}

// int main() {
//     Graph g(3);
//     g.add_edge(0,1);
//     g.add_edge(1,0);
//     const std::vector<int> expected({0, 1, -1});
//     const auto result = g.bfs_sequential(0);
//     if (expected != result){
//         // for (auto& z: expected) std::cerr << z << " ";
//         // std::cerr << "\n";
//         // for (auto& z: result) std::cerr << z << " ";
//         // std::cerr << "\n";
//         return 1;
//     }
//     return 0;
// }
