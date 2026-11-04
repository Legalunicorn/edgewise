#include "graph.hpp"
#include <gtest/gtest.h>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
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

// triangle {0,1,4} and clique {2,3,5,6,7} joined by the bridge 4-5, plus isolated vertex 8.
// the bridge is the third neighbour of both 4 and 5, so only Afforest's final phase links it.
Graph make_bridged_triangle_and_clique_graph() {
    Graph g(9);
    const std::vector<std::pair<int, int>> edges{
        {0, 1}, {0, 4},
        {1, 0}, {1, 4},
        {2, 3}, {2, 5}, {2, 6}, {2, 7},
        {3, 2}, {3, 5}, {3, 6}, {3, 7},
        {4, 0}, {4, 1}, {4, 5},
        {5, 2}, {5, 3}, {5, 4}, {5, 6}, {5, 7},
        {6, 2}, {6, 3}, {6, 5}, {6, 7},
        {7, 2}, {7, 3}, {7, 5}, {7, 6},
    };
    for (const auto& [u, v] : edges) g.add_edge(u, v);
    g.build_csr();
    return g;
}

// copies of the bridged graph, enough vertices for workers to claim several chunks
Graph make_repeated_bridged_graph(int copies) {
    const auto base = make_bridged_triangle_and_clique_graph();
    Graph g(9 * copies);
    for (int k = 0; k < copies; k++) {
        for (int u = 0; u < 9; u++) {
            for (int v : base.neighbours(u)) g.add_edge(9 * k + u, 9 * k + v);
        }
    }
    g.build_csr();
    return g;
}

// each copy has one 8-vertex component and one isolated vertex
std::vector<int> repeated_bridged_components(int copies) {
    std::vector<int> labels;
    for (int k = 0; k < copies; k++) {
        labels.insert(labels.end(), 8, 2 * k);
        labels.push_back(2 * k + 1);
    }
    return labels;
}

// 1200 copies give 10,800 vertices, more than two 4,096-vertex chunks
constexpr int kRepeatedCopies = 1200;

const std::vector<int> kDisconnectedComponents{10, 10, 10, 20, 20, 30};
const std::vector<int> kConnectedCycle{42, 42, 42};
const std::vector<int> kTwoDisconnectedPairs{1, 1, 0, 0};
const std::vector<int> kBridgedTriangleAndClique{1, 1, 1, 1, 1, 1, 1, 1, 2};

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

TEST_P(CcParallelDsuTest, BridgedTriangleAndClique) {
    const auto g = make_bridged_triangle_and_clique_graph();
    expect_same_components(g.cc_parallel_dsu(GetParam()), kBridgedTriangleAndClique);
}

TEST_P(CcParallelDsuTest, RepeatedBridgedGraphAcrossChunks) {
    const auto g = make_repeated_bridged_graph(kRepeatedCopies);
    expect_same_components(g.cc_parallel_dsu(GetParam()), repeated_bridged_components(kRepeatedCopies));
}

INSTANTIATE_TEST_SUITE_P(
    WorkerCounts,
    CcParallelDsuTest,
    ::testing::Values(std::size_t{1}, std::size_t{2}, std::size_t{4}),
    [](const ::testing::TestParamInfo<std::size_t>& info) {
        return "Workers" + std::to_string(info.param);
    }
);

class CcAfforestTest : public ::testing::TestWithParam<std::size_t> {};

TEST_P(CcAfforestTest, DisconnectedComponentsAndIsolatedVertex) {
    const auto g = make_disconnected_components_graph();
    expect_same_components(g.cc_parallel_afforest(GetParam()), kDisconnectedComponents);
}

TEST_P(CcAfforestTest, ConnectedCycle) {
    const auto g = make_connected_cycle_graph();
    expect_same_components(g.cc_parallel_afforest(GetParam()), kConnectedCycle);
}

TEST_P(CcAfforestTest, TwoDisconnectedPairs) {
    const auto g = make_two_disconnected_pairs_graph();
    expect_same_components(g.cc_parallel_afforest(GetParam()), kTwoDisconnectedPairs);
}

TEST_P(CcAfforestTest, BridgedTriangleAndClique) {
    const auto g = make_bridged_triangle_and_clique_graph();
    expect_same_components(g.cc_parallel_afforest(GetParam()), kBridgedTriangleAndClique);
}

TEST_P(CcAfforestTest, RepeatedBridgedGraphAcrossChunks) {
    const auto g = make_repeated_bridged_graph(kRepeatedCopies);
    expect_same_components(g.cc_parallel_afforest(GetParam()), repeated_bridged_components(kRepeatedCopies));
}

INSTANTIATE_TEST_SUITE_P(
    WorkerCounts,
    CcAfforestTest,
    ::testing::Values(std::size_t{1}, std::size_t{2}, std::size_t{4}),
    [](const ::testing::TestParamInfo<std::size_t>& info) {
        return "Workers" + std::to_string(info.param);
    }
);

// separate suite name: GoogleTest rejects TEST and TEST_P sharing one suite
TEST(CcAfforestInputTest, ZeroWorkersThrows) {
    const auto g = make_connected_cycle_graph();
    EXPECT_THROW(g.cc_parallel_afforest(0), std::invalid_argument);
}

}
