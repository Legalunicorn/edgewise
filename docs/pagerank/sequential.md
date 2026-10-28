# Sequential PageRank

Sequential PageRank is the correctness and performance baseline for the parallel implementation.

It returns a vector of scores and assumes a nonempty graph with CSR storage.


In this article I will explain the algorithm then provide my implementation.

## Algorithm
### Set up
Let's define some terms/parameters:
- A graph $g$,  with $n$ vertices.
- damping factor $d$ where $0 < d < 1$ such that $1-d$ is the probability of jumping  to a uniformly random vertex.
- dangling vertices are vertices with no out-degree. Let $D$ be the sum of their current scores.

### Formula for score

For each vertex `v` its next score is 
$$
\text{next}[v] =  \frac{1-d}{n} + d \left( \sum_{ \{ u \to v \mid u \in g\}} \frac{\text{score}[u]}{\text{outdegree}[u]} + \frac{D}{n} \right)
$$
::: details Brief explanation of formula
Each vertex receives a score from all of 
- random jumps with probability $1-d$.
- from vertices linking to it, which divide their current score equally to outgoing edges.
- Vertices with no outgoing edges distribute their score equally across all vertices instead, their score sums to the dangling value $D$.
:::

### Rough algorithm

1. Initialize the $score$ of each vertex to a baseline of $\frac{1}{n}$.
2. Calculate the initial dangling value which is $\frac{\text{\# vertices outdegree 0}}{n}$.
3. Repeat until maximum score change is at most the tolerance, or the iteration limit is reached
    1. Calculate _next score_ for each vertex, as well as the next dangling.
    2. Track max delta between the next score and current score for each vertex
    3. score := next score
    4. dangling := next dangling
    5. iterations++

For Edgewise I choose 
- $d = 0.85$
- max iterations is $100$
- tolerance for max score change to be $10^{-6}$


## Implementation

We implement the method
`Graph::pagerank_sequential(damping, tolerance, max_iterations)` which is declared in `graph.hpp` Refer to [graph storage](/graph-storage.md) for more details.

```cpp
std::vector<double> Graph::pagerank_sequential(
            double damping,
            double tolerance,
            std::size_t max_iterations) const {
    double init_score = 1.0 / size;
    std::size_t total_iterations = 0;
    std::vector<double> score(size, init_score);
    std::vector<double> next_score(size, init_score);
    double max_delta = 1.0, dangling = 0.0, next_dangling = 0.0;
    
    // calculate the initial dangling first
    for (std::size_t u = 0; u < size; u++){
        if (outdegree(u) == 0) dangling += score[u];
    }

    while(total_iterations < max_iterations && max_delta > tolerance){
        max_delta = 0.0;
        next_dangling = 0.0;
        for (std::size_t u = 0; u < size; u++){
            // sum of score from incoming vertices
            double incoming_score = 0.0;
            for (auto& v: incoming_neighbours(u)) {
                incoming_score += score[v] / outdegree(v);
            }
         
            // apply formula
            next_score[u] = 
                (1.0-damping) / size
                + damping * (incoming_score + dangling/size);

            if (outdegree(u) == 0) {
                next_dangling += next_score[u];
            }
            max_delta = std::max(
                max_delta,
                std::abs(next_score[u] - score[u]));
        }
        total_iterations++;
        swap(score, next_score);
        dangling = next_dangling;
    }
    return score;
}
```

Each iteration takes `O(V + E)` work, with `O(V)` additional score storage.

See [Parallel PageRank](/pagerank/parallel) for the threaded implementation.
