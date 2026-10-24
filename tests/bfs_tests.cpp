#include "graph.hpp"
#include <gtest/gtest.h>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

enum class Implementation { Sequential, V1, V2, V3, V4 };

struct BfsConfig {
    Implementation implementation; // Select the function used for this test instance.
    std::size_t worker_count; // Used only by parallel implementations.
};

// GoogleTest runs every TEST_P below once for each supplied BfsConfig.
class BfsTest : public ::testing::TestWithParam<BfsConfig> {
protected:
    std::vector<int> Run(const Graph& graph) const {
        const auto& config = GetParam(); // Read this instance's implementation and worker count.
        switch (config.implementation) {
            case Implementation::Sequential: return graph.bfs_sequential(0);
            case Implementation::V1: return graph.bfs_parallel_v1(0, config.worker_count);
            case Implementation::V2: return graph.bfs_parallel_v2(0, config.worker_count);
            case Implementation::V3: return graph.bfs_parallel_v3(0, config.worker_count);
            case Implementation::V4: return graph.bfs_parallel_v4(0, config.worker_count);
        }
        throw std::logic_error("Unknown BFS test implementation");
    }
};

TEST_P(BfsTest, DisconnectedVertices) {
    Graph g(3);
    g.add_edge(0, 1);
    g.add_edge(1, 0);
    g.build_csr(); // Finalize construction before traversal.
    const std::vector<int> expected{0, 1, -1};
    EXPECT_EQ(expected, Run(g)); // Run only this instance's BFS implementation.
}

TEST_P(BfsTest, DiamondGraph) {
    Graph g(4);
    g.add_edge(0, 1);
    g.add_edge(0, 2);
    g.add_edge(1, 3);
    g.add_edge(2, 3);
    g.build_csr();
    const std::vector<int> expected{0, 1, 1, 2};
    EXPECT_EQ(expected, Run(g));
}

TEST_P(BfsTest, Cycle) {
    Graph g(4);
    g.add_edge(0, 1);
    g.add_edge(1, 2);
    g.add_edge(2, 3);
    g.add_edge(3, 0);
    g.build_csr();
    const std::vector<int> expected{0, 1, 2, 3};
    EXPECT_EQ(expected, Run(g));
}

TEST_P(BfsTest, SingleVertexWithoutEdges) {
    Graph g(1);
    g.build_csr();
    const std::vector<int> expected{0};
    EXPECT_EQ(expected, Run(g)); // Empty outputs must still complete the copy phase and shutdown.
}

TEST_P(BfsTest, FrontierGrowsThenShrinks) {
    Graph g(8);
    for (int v = 1; v <= 5; ++v) {
        g.add_edge(0, v); // Expand the frontier from one vertex to five.
        g.add_edge(v, 6); // All five vertices compete to discover the same destination.
    }
    g.add_edge(6, 7); // Continue after shrinking back to a one-vertex frontier.
    g.build_csr();
    const std::vector<int> expected{0, 1, 1, 1, 1, 1, 2, 3};
    EXPECT_EQ(expected, Run(g));
}

// Give each generated test a readable suffix such as V4Workers3.
std::string ConfigName(const ::testing::TestParamInfo<BfsConfig>& info) {
    const auto& config = info.param;
    if (config.implementation == Implementation::Sequential) return "Sequential";
    const int version = static_cast<int>(config.implementation); // V1-V4 have enum values 1-4.
    return "V" + std::to_string(version) + "Workers" + std::to_string(config.worker_count);
}

// Adding a configuration here applies it to every graph test above.
INSTANTIATE_TEST_SUITE_P(
    AllImplementations,
    BfsTest,
    ::testing::Values(
        BfsConfig{Implementation::Sequential, 1}, // Sequential is tested once per graph.
        BfsConfig{Implementation::V1, 1},
        BfsConfig{Implementation::V1, 2},
        BfsConfig{Implementation::V1, 3},
        BfsConfig{Implementation::V2, 1},
        BfsConfig{Implementation::V2, 2},
        BfsConfig{Implementation::V2, 3},
        BfsConfig{Implementation::V3, 1},
        BfsConfig{Implementation::V3, 2},
        BfsConfig{Implementation::V3, 3},
        BfsConfig{Implementation::V4, 1},
        BfsConfig{Implementation::V4, 2},
        BfsConfig{Implementation::V4, 3}
    ),
    ConfigName
);

} // namespace
