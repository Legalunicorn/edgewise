#include "graph.hpp"
#include <gtest/gtest.h>
#include <cstddef>
#include <string>

constexpr double kTolerance = 1e-4;

class PageRankTest : public ::testing::TestWithParam<std::size_t> {
protected:
    std::vector<double> Run(const Graph& graph, double damping = 0.85,
        double tolerance = 1e-6,
        std::size_t max_iterations = 100) const {
        const auto workers = GetParam();
        if (workers == 0) {
            return graph.pagerank_sequential(damping, tolerance, max_iterations);
        }
        return graph.pagerank_parallel(workers, damping, tolerance, max_iterations);
    }
};

TEST_P(PageRankTest, DirectedCycle) {
    Graph g(3);
    g.add_edge(0, 1);
    g.add_edge(1, 2);
    g.add_edge(2, 0);
    g.build_csr();

    const auto scores = Run(g);

    ASSERT_EQ(scores.size(), 3u);
    EXPECT_NEAR(scores[0], 1.0 / 3.0, kTolerance);
    EXPECT_NEAR(scores[1], 1.0 / 3.0, kTolerance);
    EXPECT_NEAR(scores[2], 1.0 / 3.0, kTolerance);
}

TEST_P(PageRankTest, UnequalDegrees) {
    Graph g(3);
    g.add_edge(0, 1);
    g.add_edge(0, 2);
    g.add_edge(1, 0);
    g.add_edge(2, 0);
    g.build_csr();

    const auto scores = Run(g);

    ASSERT_EQ(scores.size(), 3u);
    EXPECT_NEAR(scores[0], 18.0 / 37.0, kTolerance);
    EXPECT_NEAR(scores[1], 19.0 / 74.0, kTolerance);
    EXPECT_NEAR(scores[2], 19.0 / 74.0, kTolerance);
}

TEST_P(PageRankTest, DanglingVertex) {
    Graph g(2);
    g.add_edge(0, 1);
    g.build_csr();

    const auto scores = Run(g);

    ASSERT_EQ(scores.size(), 2u);
    EXPECT_NEAR(scores[0], 20.0 / 57.0, kTolerance);
    EXPECT_NEAR(scores[1], 37.0 / 57.0, kTolerance);
}

TEST_P(PageRankTest, NoEdges) {
    Graph g(3);
    g.build_csr();

    const auto scores = Run(g);

    ASSERT_EQ(scores.size(), 3u);
    EXPECT_NEAR(scores[0], 1.0 / 3.0, kTolerance);
    EXPECT_NEAR(scores[1], 1.0 / 3.0, kTolerance);
    EXPECT_NEAR(scores[2], 1.0 / 3.0, kTolerance);
}

TEST_P(PageRankTest, SingleIteration) {
    Graph g(2);
    g.add_edge(0, 1);
    g.build_csr();

    const auto scores = Run(g, 0.85, 1e-6, 1);

    ASSERT_EQ(scores.size(), 2u);
    EXPECT_NEAR(scores[0], 0.2875, kTolerance);
    EXPECT_NEAR(scores[1], 0.7125, kTolerance);
}
TEST_P(PageRankTest, ZeroIterations) {
    Graph g(3);
    g.add_edge(0, 1);
    g.build_csr();

    const auto scores = Run(g, 0.85, 1e-6, 0);

    ASSERT_EQ(scores.size(), 3u);
    EXPECT_NEAR(scores[0], 1.0 / 3.0, kTolerance);
    EXPECT_NEAR(scores[1], 1.0 / 3.0, kTolerance);
    EXPECT_NEAR(scores[2], 1.0 / 3.0, kTolerance);
}

INSTANTIATE_TEST_SUITE_P(
    AllImplementations,
    PageRankTest,
    ::testing::Values(std::size_t{0}, std::size_t{1}, std::size_t{2},
                      std::size_t{3}, std::size_t{4}),
    [](const ::testing::TestParamInfo<std::size_t>& info) {
        if (info.param == 0) return std::string("Sequential");
        return "Workers" + std::to_string(info.param);
    }
);
