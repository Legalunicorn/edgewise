::: info Under construction
This page is under construction.
:::

# PageRank

PageRank is a well known algorithm that can measure the relative importance of a webpage using the links pointing to it. Links from higher scoring pages contribute more. Here, we can map webpages to vertices, and their links to other pages as unweighted, directed edges.

If you have not heard of this algorithm, I highly recommend readers to read through [this cs50 article](https://cs50.harvard.edu/ai/projects/2/pagerank/) (right before the getting started section) to get some context. 

PageRank can be estimated by simulating a random web surfer, or calculated mathematically through repeated score updates. This project uses repeated updates because they are deterministic and suitable for parallelization.

## Implementation


| Implementation | Focus |
|---|---|
| [Sequential PageRank](/pagerank/sequential) | Calculate every vertex's new score on one thread. |
| [Parallel PageRank](/pagerank/parallel) | Divide destination vertices among persistent workers. |


See the [PageRank benchmark results](/pagerank/results) for the comparison.
