# Connected components (CC)

A connected component is a group of vertices where every vertex can reach every other vertex through edges.
For this project, I'm interested in finding all connected components in an **undirected graph**.

## Input and output

The input is the whole graph. The output is a `vector<int>` of length `n`, where `component[v]` is the component ID of vertex `v`.

Two vertices have the same component ID iff a path connects them.

For example, given six vertices and these undirected edges:

```text
Vertices: 0, 1, 2, 3, 4, 5
Edges: (0, 1), (1, 2), (3, 4)
```
A valid output is:

```text
Vertex:       0  1  2  3  4  5
Component ID: 0  0  0  1  1  2
```

Where there are 3 components with ids `0,1,2`.
The actual ID values do not matter, so long the groupings are correct.

## Implementations

As with other algorithms in this project, all implementations use the same [CSR graph storage](/graph-storage.md). Each undirected edge is stored in both directions.


| Implementation | Brief idea |
|---|---|
| [Sequential CC using BFS](/cc/sequential-bfs) | Start BFS from each unlabelled vertex and label everything it reaches. |
| [Sequential CC using DSU](/cc/sequential-dsu) | Union the endpoints of every edge, then find each vertex's final representative. |
| [Parallel CC using concurrent DSU](/cc/parallel-dsu) | Workers claim chunks of vertices and union their edges using a shared concurrent DSU. |
| [Afforest](/cc/afforest) | Link one or two neighbours per vertex first, then skip vertices that are already in the largest component. |

DSU is a data structure closely related to connectivity, their implementations are explained separately in [Sequential DSU](/cc/dsu/sequential) and [Concurrent DSU](/cc/dsu/concurrent).

Sequential CC using BFS is the simplest version and is used to check every other result. Sequential DSU was faster on every graph, so the results compare against DSU instead.

## Results

See the [CC benchmark results](/cc/results) for timings of parallel DSU and Afforest against sequential DSU. In short, Afforest is slightly slower than parallel DSU on small, low-degree graphs, and is up to 1.94× faster on larger or denser ones.
