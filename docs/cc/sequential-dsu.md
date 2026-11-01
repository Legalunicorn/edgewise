# Sequential CC using DSU

This implementation constructs the CC by performing union on every edge on the graph. Once that is done, we find the component representative of each vertex using the `find()` method by the DSU struct.


As with other CC implementations here we assumes an undirected graph and use the [sequential DSU](/cc/dsu/sequential) implementation.

## Implementation

With DSU doing the bulk of the heavy work, the implementation is fairly short here.

```cpp
std::vector<int> Graph::cc_sequential_dsu() const {
    SequentialDSU dsu(size);
    // union all edges
    for (int u = 0; u < size; u++) {
        for (int v : neighbours(u)) {
            dsu.union_set(u, v);
        }
    }
    // find the ID of each vertex
    std::vector<int> component(size);
    for (int u = 0; u < size; u++) component[u] = dsu.find(u);
    return component;
}
```


With path compression and union by size, the time complexity is $O((V + E) \alpha (V))$, where $\alpha(V)$ grows very slowly, and can be treated as $O(1)$ in most practical senses. Additional space is $O(V)$.

See [Parallel CC using concurrent DSU](/cc/parallel-dsu) for the threaded implementation.
