# Sequential BFS

I used sequential BFS as the correctness and performance baseline for the parallel implementations.

It traverses the same graph and returns a distance vector, with `-1` for unreachable vertices.

## Implementation
The following implementation uses a standard BFS queue.

```cpp
// Defines the Graph method declared in graph.hpp.
// - int s: source vertex of bfs
// - returns a vector of distances
std::vector<int> Graph::bfs_sequential(int s) const{
    std::vector<int> distance(size, -1);
    std::queue<int> frontier;
    frontier.push(s);
    distance[s] = 0;
    int curr_dist = 0;
    while(frontier.size() > 0) {
        int len = frontier.size();
        curr_dist++;
        for (size_t i = 0; i < len; i++){
            int u = frontier.front();
            frontier.pop();
            for (auto& v: neighbours(u)){
                if (distance[v] != -1) continue;
                frontier.push(v);
                distance[v] = curr_dist;
            }
        }
    }
    return distance;
};
```
:::info
- `distance[i]` returns the distance from the source `s`, which would be `-1` if unreachable.
- `frontier` is the BFS queue. At the start of each outer loop iteration, it contains one distance layer. During that iteration, it can contain the remaining current-layer vertices and newly discovered next-layer vertices.
- `Graph` is a custom struct that provides a basic graph API such as `neighbours(u)` which returns a span containing the vertices `u` points to.
:::

The worst-case time complexity is `O(V + E)`, with `O(V)` additional space.

See [Parallel BFS](/bfs/v1-mutex) for the threaded implementations.
