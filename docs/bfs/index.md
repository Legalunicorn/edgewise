# Breadth-first search

Breadth-first search (BFS) finds the shortest path distances from a source vertex in an unweighted graph. 

In this project, each implementation here returns a distance vector, with `-1` for unreachable vertices.

I started with a simple sequential implementation, then made parallel iterations, each attempting to improve the previous version. All of them traverse the same [CSR graph storage](/graph-storage.md) and compute the same distances.


## Frontier

A **frontier** is one BFS distance layer. Vertices within that layer can be processed concurrently, but the next layer must be completed before it is expanded.

![Animation of parallel BFS with four workers expanding ring by ring from a source vertex](/animations/parallel-bfs-radial.svg)

The animation shows V3 on a generated graph, with each ring being one frontier. The frontier is split into four contiguous slices, one per worker, and each colour marks the worker responsible for a vertex. 

A red ring marks a lost compare-and-swap: two workers saw the same vertex as unvisited, and only one claim succeeded. This is applied in [V2](/bfs/v2-atomic).

Finished vertices turn grey, and the last frame shows the BFS tree. [V3: Persistent workers](/bfs/v3-persistent-workers) walks through the same steps on a smaller graph.


::: tip Note
Generative AI was used to create the visualizations for this project.
:::

## Implementations

I recommend reading in this order. They're written as a tutorial style because I found it enjoyable, and it also solidifies my understanding.

| Implementation | Focus |
|---|---|
| [Sequential BFS](/bfs/sequential) | The single-threaded correctness and performance baseline. |
| [V1: Mutex discovery](/bfs/v1-mutex) | Split each frontier among workers and protect discovery with one shared mutex. |
| [V2: Atomic discovery](/bfs/v2-atomic) | Replace the shared discovery mutex with atomic operations. |
| [V3: Persistent workers](/bfs/v3-persistent-workers) | Reuse workers across layers instead of creating them repeatedly. |
| [V4: Parallel frontier assembly](/bfs/v4-parallel-frontier-assembly) | Combine worker outputs into the next frontier in parallel. |

## Results

See the [benchmark results](/bfs/results). Each version is an experiment: an attempted optimization does not necessarily make BFS faster.
