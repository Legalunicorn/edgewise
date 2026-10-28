#include "graph.hpp"
#include <algorithm>
#include <cmath>
#include <thread>
#include <barrier>

std::vector<double> Graph::pagerank_parallel(std::size_t worker_count, double damping,
        double tolerance, std::size_t max_iterations) const{
    double init_score = 1.0 / size;
    std::size_t total_iterations = 0;
    
    const auto total_active = static_cast<ptrdiff_t>(worker_count +1);
    // persistent worker threads use barriers
    std::barrier<> start_iteration(total_active);
    std::barrier<> end_iteration(total_active);

    std::vector<double> pr(size, init_score);
    std::vector<double> next_pr(size, init_score);
    std::vector<double> worker_dangling(worker_count, 0.0);

    // init the delta to be high so it wont pass after the first round while computing dangling
    std::vector<double> worker_delta(worker_count, 1.0);
    double max_delta = 1.0, dangling = 0.0;
    bool done = false;

    // we have to init next dangling because each iteration requires the dangling to be computed first
    bool init_next_dangling = false;

    // calculate PR for range of g[start:end)
    auto compute_pr_range = [&](std::size_t start, std::size_t end, std::size_t id){
        double local_max_delta = 0.0;
        double local_next_dangling = 0.0;
        for (std::size_t u = start; u < end; u++){
            double score = 0.0;
            for (auto& v: incoming_neighbours(u)){
                score += pr[v] / outdegree(v);
            }
            next_pr[u] = (1.0-damping) / size + damping * (score + dangling/size);
            if (outdegree(u) == 0) local_next_dangling += next_pr[u];
            local_max_delta = std::max(local_max_delta, std::abs(next_pr[u] - pr[u]));

        }
        worker_dangling[id] = local_next_dangling;
        worker_delta[id] = local_max_delta;
    };

    // to execute ONCE before the first iteraation 
    // calculate the initial dangling score of the graph
    auto init_dangling = [&](std::size_t start, std::size_t end, std::size_t id) {
        double local_next_dangling = 0.0;
        for (std::size_t u = start ; u < end; u++){
            if (outdegree(u) == 0) local_next_dangling += pr[u];
        }
        worker_dangling[id] = local_next_dangling;
    };


    auto worker_loop = [&](std::size_t id) -> void {
        while(true) {
            // logic for barriers
            start_iteration.arrive_and_wait();
            if (done) return;
            const std::size_t begin = size * id / worker_count;
            const std::size_t end = size * (id+1) / worker_count;
            if (init_next_dangling) compute_pr_range(begin, end, id);
            else init_dangling(begin, end, id);
            end_iteration.arrive_and_wait();
        }
    };

    // start the iterations
    std::vector<std::jthread> workers;
    for (std::size_t id = 0; id < worker_count; id++){
        workers.emplace_back(worker_loop, id);
    }

    // main thread -> coordinates the start and stop of iterations
    while(total_iterations < max_iterations && max_delta > tolerance){
        start_iteration.arrive_and_wait();
        end_iteration.arrive_and_wait();
        max_delta = 0.0;
        dangling = 0.0;
        for (std::size_t w = 0; w < worker_count; w++){
            max_delta = std::max(max_delta, worker_delta[w]);
            dangling += worker_dangling[w];
        }
        if (!init_next_dangling) {
            // initial dangling computation complete, next rounds is actuall pr calculations
            init_next_dangling = true;
        } else{
            total_iterations++;
            pr.swap(next_pr);
        }
    }
    done = true;
    start_iteration.arrive_and_wait();
    
    // join all worker th    
    for (auto& w: workers) w.join();
    return pr;
}
