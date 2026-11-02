#include "graph_loader.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <cstddef>
#include <vector>

TEST(GraphLoaderTest, LoadsUndirectedEdgeList) {
    const auto g = load_edge_list("tests/data/loader_undirected.txt", true);

    // for this specific file we set 6 vertices
    ASSERT_EQ(g.size, 6);

    std::size_t sum_outdegree = 0;
    std::vector<std::size_t> degrees;
    for (int u = 0; u < g.size; u++) {
        sum_outdegree += g.outdegree(u);
        degrees.push_back(g.outdegree(u));
    }

    // for this file its set to 11
    EXPECT_EQ(sum_outdegree, 11);

    // check degrees without depending on the assigned internal IDs.
    std::sort(degrees.begin(), degrees.end());
    const std::vector<std::size_t> expected{1, 1, 1, 2, 3, 3};
    EXPECT_EQ(degrees, expected);

    const auto component = g.cc_sequential_bfs();
    for (int label : component) {
        EXPECT_EQ(label, component.front());
    }
}
