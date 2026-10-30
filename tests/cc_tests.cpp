#include "graph.hpp"
#include <gtest/gtest.h>
#include <cstddef>
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

TEST(CcTest, DisconnectedComponentsAndIsolatedVertex) {
    Graph g(6);
    g.add_edge(0, 1);
    g.add_edge(1, 0);
    g.add_edge(1, 2);
    g.add_edge(2, 1);
    g.add_edge(3, 4);
    g.add_edge(4, 3);
    g.build_csr();

    const std::vector<int> expected{10, 10, 10, 20, 20, 30};
    expect_same_components(g.cc_sequential_bfs(), expected);
}

TEST(CcTest, ConnectedCycle) {
    Graph g(3);
    g.add_edge(0, 1);
    g.add_edge(1, 0);
    g.add_edge(1, 2);
    g.add_edge(2, 1);
    g.add_edge(2, 0);
    g.add_edge(0, 2);
    g.build_csr();

    const std::vector<int> expected{42, 42, 42};
    expect_same_components(g.cc_sequential_bfs(), expected);
}


TEST(CcTest, TwoDisconnectedPairs){
    Graph g(4);
    g.add_edge(0, 1);
    g.add_edge(1, 0);
    g.add_edge(2, 3);
    g.add_edge(3 ,2);
    g.build_csr();
    const std::vector<int> expected{1, 1, 0, 0};
    expect_same_components(g.cc_sequential_bfs(), expected);
}


}
