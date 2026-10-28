# Parallel PageRank

Parallel PageRank uses the same formula and stopping condition as [sequential PageRank](/pagerank/sequential). It assumes a nonempty graph with CSR storage and a positive worker count.

The ideas used follow closely from what I learnt from [BFS parallel V3](/bfs/v3-persistent-workers) with the user of persistent workers. 

## Brief idea

Each worker gets a range of destination vertices and calculates their next scores using incoming edges. The worker threads are created once and reused across iterations.

Each round has four steps:
1. The coordinator releases the workers through the start barrier.
2. Each worker calculates its next scores, maximum absolute score change, and next dangling-score sum.
3. Everyone waits at the finish barrier.
4. The coordinator combines the worker results and swaps the current and next score buffers.

The first round only computes the initial dangling score. It does not count as a PageRank iteration. Later rounds calculate the next dangling score alongside the next vertex scores.

## Implementation

This is my `Graph::pagerank_parallel` implementation with clearer variable names. The coordinator participates in both barriers, so each barrier has `worker_count + 1` participants.

```cpp
#include "graph.hpp"
#include <algorithm>
#include <cmath>
#include <thread>
#include <barrier>

std::vector<double> Graph::pagerank_parallel(std::size_t worker_count, double damping,
        double tolerance, std::size_t max_iterations) const{
    double initial_score = 1.0 / size;
    std::size_t iterations = 0;
    
    const auto participants = static_cast<std::ptrdiff_t>(worker_count +1);
    std::barrier<> start_round(participants);
    std::barrier<> finish_round(participants);

    std::vector<double> scores(size, initial_score);
    std::vector<double> next_scores(size, initial_score);
    std::vector<double> worker_dangling_sums(worker_count, 0.0);
    // Keep the loop running after the initial dangling-only round.
    std::vector<double> worker_max_deltas(worker_count, 1.0);
    double max_delta = 1.0, dangling_score = 0.0;
    bool stop_workers = false;
    bool initial_dangling_ready = false;
    // Update this worker's vertices and collect local results.
    auto compute_next_scores = [&](std::size_t begin, std::size_t end, std::size_t worker_id){
        double local_max_delta = 0.0;
        double local_dangling_sum = 0.0;
        for (std::size_t u = begin; u < end; u++){
            double incoming_score = 0.0;
            for (auto& v: incoming_neighbours(u)){
                incoming_score += scores[v] / outdegree(v);
            }
            next_scores[u] = (1.0-damping) / size + damping * (incoming_score + dangling_score/size);
            if (outdegree(u) == 0) local_dangling_sum += next_scores[u];
            local_max_delta = std::max(local_max_delta, std::abs(next_scores[u] - scores[u]));

        }
        worker_dangling_sums[worker_id] = local_dangling_sum;
        worker_max_deltas[worker_id] = local_max_delta;
    };
    // Sum initial scores of dangling vertices in this range.
    auto compute_initial_dangling = [&](std::size_t begin, std::size_t end, std::size_t worker_id) {
        double local_dangling_sum = 0.0;
        for (std::size_t u = begin ; u < end; u++){
            if (outdegree(u) == 0) local_dangling_sum += scores[u];
        }
        worker_dangling_sums[worker_id] = local_dangling_sum;
    };

    // Reuse each worker for initialization and later iterations.
    auto worker_loop = [&](std::size_t worker_id) -> void {
        while(true) {
            start_round.arrive_and_wait();
            if (stop_workers) return;
            const std::size_t begin = size * worker_id / worker_count;
            const std::size_t end = size * (worker_id+1) / worker_count;
            if (initial_dangling_ready) compute_next_scores(begin, end, worker_id);
            else compute_initial_dangling(begin, end, worker_id);
            finish_round.arrive_and_wait();
        }
    };
    std::vector<std::jthread> workers;
    for (std::size_t worker_id = 0; worker_id < worker_count; worker_id++){
        workers.emplace_back(worker_loop, worker_id);
    }
    while(iterations < max_iterations && max_delta > tolerance){
        start_round.arrive_and_wait();
        finish_round.arrive_and_wait();
        // All workers have finished; combine their results.
        max_delta = 0.0;
        dangling_score = 0.0;
        for (std::size_t worker = 0; worker < worker_count; worker++){
            max_delta = std::max(max_delta, worker_max_deltas[worker]);
            dangling_score += worker_dangling_sums[worker];
        }
        if (!initial_dangling_ready) {
            initial_dangling_ready = true;
        } else{
            iterations++;
            scores.swap(next_scores);
        }
    }
    // Release workers from the start barrier so they can exit.
    stop_workers = true;
    start_round.arrive_and_wait();
    for (auto& worker: workers) worker.join();
    return scores;
}
```

See the [PageRank results](/pagerank/results) for the comparison with sequential execution.
