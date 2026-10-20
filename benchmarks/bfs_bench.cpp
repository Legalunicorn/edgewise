#include "generate_graph.hpp"
#include <chrono>
#include <iostream>

int main() {
    Graph g = gen_graph::random_connected(200'000, 800'000, 67);
    //csv columns
    std::cout << "implementation,workers,run,time_ms\n";
    
    for (std::size_t run = 1; run <= 5; run++){
        const auto seq_start = std::chrono::steady_clock::now();
        auto expected = g.bfs_sequential(0);
        const auto seq_end = std::chrono::steady_clock::now();
        const std::chrono::duration<double, std::milli> seq_time = seq_end - seq_start;
        std::cout << "sequential,1," << run << "," << seq_time.count() << "\n";
        for (std::size_t w = 1; w <= 16; w++) {
            const auto start = std::chrono::steady_clock::now();
            auto result = g.bfs_parallel(0, w);
            const auto end = std::chrono::steady_clock::now();
            const std::chrono::duration<double, std::milli> parallel_time = end - start;
            if (result != expected){
                std::cerr << "Parallel BFS result is wrong\n";
                return 1;
            }
            std::cout << "parallel," << w << "," << run << "," << parallel_time.count() << "\n";

        }
    }
}
