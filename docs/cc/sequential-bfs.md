# Sequential CC using BFS

This implementation runs BFS from each unlabelled vertex. All vertices reached by that BFS receive the same component ID, then the next unlabelled vertex starts a new component.

Here we assume an undirected graph and return a vector of component IDs.

## Implementation

This follows the same queue-based approach as [sequential BFS](/bfs/sequential), but records component IDs instead of distances. 



```cpp
std::vector<int> Graph::cc_sequential_bfs() const {
    // mark -1 as unvisited, since we use 0 as minimum component id
    std::vector<int> component(size, -1);
    int curr_id = 0;
    for (int u = 0; u < size; u++) {
        // visited already -> skip
        if (component[u] != -1) continue;

        // simple bfs implementation
        std::queue<int> frontier;
        frontier.push(u);
        component[u] = curr_id;

        while (frontier.size() > 0) {
            int u = frontier.front();
            frontier.pop();

            for (int v : neighbours(u)) {
                if (component[v] != -1) continue;
                // label on enqueue so each vertex enters the queue once.
                frontier.push(v);
                component[v] = curr_id;
            }
        }
        curr_id++;
    }
    return component;
}
```

The time complexity is `O(V + E)`, with `O(V)` additional space.

See [Sequential CC using DSU](/cc/sequential-dsu) for the other sequential baseline.
