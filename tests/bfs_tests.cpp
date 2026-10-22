#include "graph.hpp"
#include <vector>
#include <gtest/gtest.h>


TEST(BFS, DisconnectedVertices) {
    Graph g(3);
    g.add_edge(0,1);
    g.add_edge(1,0);
    g.build_csr();
    const std::vector<int> expected({0,1,-1});
    EXPECT_EQ(expected, g.bfs_sequential(0));
    EXPECT_EQ(expected, g.bfs_parallel_v1(0,1));
    EXPECT_EQ(expected, g.bfs_parallel_v1(0,2));
    EXPECT_EQ(expected, g.bfs_parallel_v1(0,3));
    EXPECT_EQ(expected, g.bfs_parallel_v2(0,1));
    EXPECT_EQ(expected, g.bfs_parallel_v2(0,2));
    EXPECT_EQ(expected, g.bfs_parallel_v2(0,3));
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
    g.build_csr();
    const std::vector<int> expected({0,1,1,2});
    const auto result = g.bfs_sequential(0);
    EXPECT_EQ(expected, result);
    EXPECT_EQ(expected, g.bfs_parallel_v1(0,1));
    EXPECT_EQ(expected, g.bfs_parallel_v1(0,2));
    EXPECT_EQ(expected, g.bfs_parallel_v1(0,3));
    EXPECT_EQ(expected, g.bfs_parallel_v2(0,1));
    EXPECT_EQ(expected, g.bfs_parallel_v2(0,2));
    EXPECT_EQ(expected, g.bfs_parallel_v2(0,3));
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
    g.build_csr();
    const std::vector<int> expected({0,1,2,3});
    const auto result = g.bfs_sequential(0);
    EXPECT_EQ(expected, result);
    EXPECT_EQ(expected, g.bfs_parallel_v1(0,1));
    EXPECT_EQ(expected, g.bfs_parallel_v1(0,2));
    EXPECT_EQ(expected, g.bfs_parallel_v1(0,3));
    EXPECT_EQ(expected, g.bfs_parallel_v2(0,1));
    EXPECT_EQ(expected, g.bfs_parallel_v2(0,2));
    EXPECT_EQ(expected, g.bfs_parallel_v2(0,3));
    EXPECT_EQ(expected, g.bfs_parallel_v3(0,1));
    EXPECT_EQ(expected, g.bfs_parallel_v3(0,2));
    EXPECT_EQ(expected, g.bfs_parallel_v3(0,3));
}
