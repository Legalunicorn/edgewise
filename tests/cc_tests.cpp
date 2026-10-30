#include "graph.hpp"
#include <gtest/gtest.h>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void expect_same_components(const std::vector<int>& actual, const std::vector<int>& expected) {
    ASSERT_EQ(actual.size(), expected.size());
    // we need a 1-1 mapping
    std::unordered_map<int, int> front;
    std::unordered_map<int, int> back;
    for (std::size_t i = 0; i < actual.size(); ++i) {
        const auto forward = front.try_emplace(actual[i], expected[i]).first;
        ASSERT_EQ(forward->second, expected[i]) << "Different components merged at vertex " << i;
        const auto reverse = back.try_emplace(expected[i], actual[i]).first;
        ASSERT_EQ(reverse->second, actual[i]) << "One component split at vertex " << i;
    }
}

Graph make_disconnected_components_graph() {
    Graph g(6);
    g.add_edge(0, 1);
    g.add_edge(1, 0);
    g.add_edge(1, 2);
    g.add_edge(2, 1);
    g.add_edge(3, 4);
    g.add_edge(4, 3);
    g.build_csr();
    return g;
}

Graph make_connected_cycle_graph() {
    Graph g(3);
    g.add_edge(0, 1);
    g.add_edge(1, 0);
    g.add_edge(1, 2);
    g.add_edge(2, 1);
    g.add_edge(2, 0);
    g.add_edge(0, 2);
    g.build_csr();
    return g;
}

Graph make_two_disconnected_pairs_graph() {
    Graph g(4);
    g.add_edge(0, 1);
    g.add_edge(1, 0);
    g.add_edge(2, 3);
    g.add_edge(3, 2);
    g.build_csr();
    return g;
}

const std::vector<int> kDisconnectedComponents{10, 10, 10, 20, 20, 30};
const std::vector<int> kConnectedCycle{42, 42, 42};
const std::vector<int> kTwoDisconnectedPairs{1, 1, 0, 0};

TEST(CcBfsTest, DisconnectedComponentsAndIsolatedVertex) {
    const auto g = make_disconnected_components_graph();
    expect_same_components(g.cc_sequential_bfs(), kDisconnectedComponents);
}

TEST(CcBfsTest, ConnectedCycle) {
    const auto g = make_connected_cycle_graph();
    expect_same_components(g.cc_sequential_bfs(), kConnectedCycle);
}

TEST(CcBfsTest, TwoDisconnectedPairs) {
    const auto g = make_two_disconnected_pairs_graph();
    expect_same_components(g.cc_sequential_bfs(), kTwoDisconnectedPairs);
}

TEST(CcDsuTest, DisconnectedComponentsAndIsolatedVertex) {
    const auto g = make_disconnected_components_graph();
    expect_same_components(g.cc_sequential_dsu(), kDisconnectedComponents);
}

TEST(CcDsuTest, ConnectedCycle) {
    const auto g = make_connected_cycle_graph();
    expect_same_components(g.cc_sequential_dsu(), kConnectedCycle);
}

TEST(CcDsuTest, TwoDisconnectedPairs) {
    const auto g = make_two_disconnected_pairs_graph();
    expect_same_components(g.cc_sequential_dsu(), kTwoDisconnectedPairs);
}

class CcParallelDsuTest : public ::testing::TestWithParam<std::size_t> {};

TEST_P(CcParallelDsuTest, DisconnectedComponentsAndIsolatedVertex) {
    const auto g = make_disconnected_components_graph();
    expect_same_components(g.cc_parallel_dsu(GetParam()), kDisconnectedComponents);
}

TEST_P(CcParallelDsuTest, ConnectedCycle) {
    const auto g = make_connected_cycle_graph();
    expect_same_components(g.cc_parallel_dsu(GetParam()), kConnectedCycle);
}

TEST_P(CcParallelDsuTest, TwoDisconnectedPairs) {
    const auto g = make_two_disconnected_pairs_graph();
    expect_same_components(g.cc_parallel_dsu(GetParam()), kTwoDisconnectedPairs);
}

INSTANTIATE_TEST_SUITE_P(
    WorkerCounts,
    CcParallelDsuTest,
    ::testing::Values(std::size_t{1}, std::size_t{2}, std::size_t{4}),
    [](const ::testing::TestParamInfo<std::size_t>& info) {
        return "Workers" + std::to_string(info.param);
    }
);

}
