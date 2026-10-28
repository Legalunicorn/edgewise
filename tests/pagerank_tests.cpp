#include "graph.hpp"
#include <gtest/gtest.h>

constexpr double kTolerance = 1e-4;

TEST(PageRankTest, DirectedCycle) {
    Graph g(3);
    g.add_edge(0, 1);
    g.add_edge(1, 2);
    g.add_edge(2, 0);
    g.build_csr();

    const auto scores = g.pagerank_sequential();

    ASSERT_EQ(scores.size(), 3u);
    EXPECT_NEAR(scores[0], 1.0 / 3.0, kTolerance);
    EXPECT_NEAR(scores[1], 1.0 / 3.0, kTolerance);
    EXPECT_NEAR(scores[2], 1.0 / 3.0, kTolerance);
}

TEST(PageRankTest, UnequalDegrees) {
    Graph g(3);
    g.add_edge(0, 1);
    g.add_edge(0, 2);
    g.add_edge(1, 0);
    g.add_edge(2, 0);
    g.build_csr();

    const auto scores = g.pagerank_sequential();

    ASSERT_EQ(scores.size(), 3u);
    EXPECT_NEAR(scores[0], 18.0 / 37.0, kTolerance);
    EXPECT_NEAR(scores[1], 19.0 / 74.0, kTolerance);
    EXPECT_NEAR(scores[2], 19.0 / 74.0, kTolerance);
}

TEST(PageRankTest, DanglingVertex) {
    Graph g(2);
    g.add_edge(0, 1);
    g.build_csr();

    const auto scores = g.pagerank_sequential();

    ASSERT_EQ(scores.size(), 2u);
    EXPECT_NEAR(scores[0], 20.0 / 57.0, kTolerance);
    EXPECT_NEAR(scores[1], 37.0 / 57.0, kTolerance);
}

TEST(PageRankTest, NoEdges) {
    Graph g(3);
    g.build_csr();

    const auto scores = g.pagerank_sequential();

    ASSERT_EQ(scores.size(), 3u);
    EXPECT_NEAR(scores[0], 1.0 / 3.0, kTolerance);
    EXPECT_NEAR(scores[1], 1.0 / 3.0, kTolerance);
    EXPECT_NEAR(scores[2], 1.0 / 3.0, kTolerance);
}

TEST(PageRankTest, SingleIteration) {
    Graph g(2);
    g.add_edge(0, 1);
    g.build_csr();

    const auto scores = g.pagerank_sequential(0.85, 1e-6, 1);

    ASSERT_EQ(scores.size(), 2u);
    EXPECT_NEAR(scores[0], 0.2875, kTolerance);
    EXPECT_NEAR(scores[1], 0.7125, kTolerance);
}
