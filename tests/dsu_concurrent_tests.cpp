#include "dsu_concurrent.hpp"
#include <gtest/gtest.h>
#include <barrier>
#include <thread>
#include <vector>

TEST(ConcurrentDsuTest, InitiallySeparate) {
    ConcurrentDSU dsu(6);
    ASSERT_EQ(dsu.par.size(), 6u);
    for (int v = 0; v < 6; ++v) {
        EXPECT_EQ(dsu.find(v), v);
    }
}

// test correctness for sequnetial
TEST(ConcurrentDsuTest, MergesAndKeepsOtherSetsSeparate) {
    ConcurrentDSU dsu(6);
    dsu.union_set(3, 2);
    dsu.union_set(1, 0);
    dsu.union_set(3, 1);
    dsu.union_set(0, 3);
    dsu.union_set(2, 2);
    dsu.union_set(5, 4);

    for (int v = 0; v < 4; ++v) {
        EXPECT_EQ(dsu.find(v), 0);
    }
    EXPECT_EQ(dsu.find(4), 4);
    EXPECT_EQ(dsu.find(5), 4);
}

TEST(ConcurrentDsuTest, CompressesParentChain) {
    ConcurrentDSU dsu(8);
    for (int v = 7; v > 0; --v) {
        dsu.union_set(v, v - 1);
    }

    ASSERT_EQ(dsu.par[7].load(), 6);
    EXPECT_EQ(dsu.find(7), 0);
    for (int v = 0; v < 8; ++v) {
        EXPECT_EQ(dsu.par[v].load(), 0);
    }
}

TEST(ConcurrentDsuTest, ConcurrentMergesOfSameRoot) {
    for (int run = 0; run < 32; ++run) {
        ConcurrentDSU dsu(9);
        std::barrier start(2);
        std::jthread first([&] {
            start.arrive_and_wait();
            dsu.union_set(8, 1);
        });
        std::jthread second([&] {
            start.arrive_and_wait();
            dsu.union_set(8, 3);
        });
        first.join();
        second.join();

        EXPECT_EQ(dsu.find(8), 1);
        EXPECT_EQ(dsu.find(3), 1);
        EXPECT_EQ(dsu.find(1), 1);
        EXPECT_EQ(dsu.find(0), 0);
    }
}

TEST(ConcurrentDsuTest, ConcurrentFindsAndUnions) {
    constexpr int vertex_count = 256;
    constexpr int worker_count = 4;
    ConcurrentDSU dsu(vertex_count);
    // barrier to all start togeter
    std::barrier start(worker_count);
    std::vector<std::jthread> workers;

    for (int id = 0; id < worker_count; ++id) {
        // use barriers, then all start togeter
        workers.emplace_back([&, id] {
            start.arrive_and_wait();
            // union based on parity
            for (int v = id; v + 2 < vertex_count; v += worker_count) {
                dsu.union_set(v, v + 2);
                dsu.find(v);
            }
            for (int v = vertex_count - 1 - id; v >= 0; v -= worker_count) {
                dsu.find(v);
            }
        });
    }
    for (auto& worker : workers) worker.join();

    // result is determinisstic for concurrent DSU
    // root is defined to be the smallest id
    // in this case of pairty its just {0, 1}
    for (int v = 0; v < vertex_count; ++v) {
        EXPECT_EQ(dsu.find(v), v % 2);
    }
}
