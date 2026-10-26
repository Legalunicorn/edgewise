::: warning Under construction
This page is under construction.
:::

# Graph storage

## The Graph type

`graph.hpp` defines the `Graph` struct used throughout the project.

Its public API is listed below. BFS methods are declared here, with their implementations in separate `.cpp` files. 

The graph uses Compressed Sparse Row (CSR) storage, which is elaborated below.


### API

::: info Note 
`std::` is omitted from the table for readability.
::: 

| Method / constructor | Description |
|---|---|
| `explicit Graph(size_t n)` | Create a graph with vertices `0` to `n - 1`. |
| `void add_edge(int u, int v)` | Add the directed edge `u -> v` before finalization. |
| `void build_csr()` | Convert the construction adjacency list to CSR and release it. Later calls do nothing. |
| `span<const int> neighbours(int u) const` | Return a read-only view of `u`'s neighbours after finalization, without copying. |
| `vector<int> bfs_sequential(int s) const` | Run sequential BFS from source `s`. |
| `vector<int> bfs_parallel_v1(int s, size_t worker_count) const` | Run parallel BFS with a shared discovery mutex. |
| `vector<int> bfs_parallel_v2(int s, size_t worker_count) const` | Run parallel BFS with atomic discovery. |
| `vector<int> bfs_parallel_v3(int s, size_t worker_count) const` | Run parallel BFS with persistent workers. |
| `vector<int> bfs_parallel_v4(int s, size_t worker_count) const` | Run parallel BFS with persistent workers and parallel frontier assembly. |

- All BFS methods return distances from `s`, with `-1` for unreachable vertices.
- `worker_count` is the number of worker threads to create.

- All BFS implementations use the same `Graph` type with CSR storage.
- Vertex IDs are zero-based integers.

## Why CSR?
Initially,  I used an adjacency list `vector<vector<int>>` to represent the graph. This is easy to build, and a common representation commonly used in competitive programming. 

CSR puts all neighbour IDs into one contiguous array. 
This **reduces allocation overhead and can improve cache locality when scanning edges**. 

Between parallel V3 and V4, I switched from adjacency list to CSR and saw a speed up of roughly **~3x** for sequential and parallel implementations.


### Fixed after finalization
In this implementation, edges cannot be added after calling `build_csr()`. This suits our BFS algorithms, which only read the graph. CSR can support changes, but updating its packed arrays is more costly.

## Representation

Consider this directed graph:

```text
0 -> {1, 2}
1 -> {2}
2 -> {}
```
An adjacency list representation stores it as such 
```cpp
vector<vector<int>> list = {{1,2}, {2}, {}};
```
We take the adjacency list values and flatten it to a 1D vector, `neighbours`. Then, we also store offsets to store the range of `neighbours` indices for each vertex.

```text
offsets:    [0, 2, 3, 3]
neighbours: [1, 2, 2]
```

- `offsets[u]` is where vertex `u`'s neighbours begin.
- `offsets[u + 1]` is where they end, excluding that position.

There are `V + 1` offsets. The final offset is the total number of stored neighbour entries.

## Building and reading the graph

The following builds and reads the same graph shown above:


```cpp
// create a graph of n = 3
Graph g(3);
// add_edge adds a directed edge
g.add_edge(0, 1); 
g.add_edge(0, 2);
g.add_edge(1, 2);
// build graph, can only be done once
g.build_csr();

for (int v : g.neighbours(0)) { // Read neighbours 1 and 2 without copying.
    // Process v
}
```

`add_edge(u, v)` adds only `u -> v`. For an undirected edge, the caller adds both directions.

Before finalization, edges are collected in an adjacency list. `build_csr()` converts it to CSR and releases that list. Later calls do nothing; adding edges after finalization throws an exception. The finalized graph can be reused across traversals.

`neighbours(u)` returns a `span<const int>`: a read-only view into the stored array, with no allocation or copying. Workers can read this storage concurrently; each BFS call keeps its distances and frontier separately.
