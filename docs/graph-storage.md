# Graph storage

## The Graph type

`graph.hpp` defines the `Graph` struct used throughout the project.

Its public API is listed below. Algorithm methods are declared here, with their implementations in separate `.cpp` files. 

The graph uses Compressed Sparse Row (CSR) storage, which is elaborated below.


### API

::: info Note
`std::` is omitted for readability. Algorithm methods below use shortened signatures.
:::

#### Graph methods

| Method / constructor | Description |
|---|---|
| `explicit Graph(size_t n)` | Create vertices `0` to `n - 1`. |
| `void add_edge(int u, int v)` | Add `u -> v` before finalization. |
| `void build_csr()` | Build outgoing and incoming CSR, then release both adjacency lists. Later calls do nothing. |
| `span<const int> neighbours(int u) const` | Read outgoing neighbours without copying. |
| `span<const int> incoming_neighbours(int u) const` | Read incoming neighbours without copying. |
| `size_t outdegree(int u) const` | Count outgoing edges. |
| `size_t indegree(int u) const` | Count incoming edges. |

Call `build_csr()` before reading neighbours or degrees. Vertex IDs are zero-based integers.

#### Algorithm methods

| Method | Description |
|---|---|
| `bfs_sequential(s)` | Sequential BFS. |
| `bfs_parallel_v1(s, worker_count)` | BFS with mutex-protected discovery. |
| `bfs_parallel_v2(s, worker_count)` | BFS with atomic discovery. |
| `bfs_parallel_v3(s, worker_count)` | BFS with persistent workers. |
| `bfs_parallel_v4(s, worker_count)` | BFS with parallel frontier assembly. |
| `pagerank_sequential(damping, tolerance, max_iterations)` | Sequential PageRank. |
| `pagerank_parallel(worker_count, damping, tolerance, max_iterations)` | PageRank with persistent workers. |

BFS returns a `vector<int>` of distances from `s`, with `-1` for unreachable vertices. PageRank returns a `vector<double>` of scores. `worker_count` is the number of worker threads.

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
