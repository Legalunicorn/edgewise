# V4: Parallel frontier assembly

- V4 distributes frontier assembly across workers.
- After scanning, each worker reports the size of its private output. A prefix sum assigns each buffer a separate destination range in `next`. Workers then copy their buffers into those ranges concurrently.

```text
Output sizes:         3       2       4
Destination ranges: [0, 3)  [3, 5)  [5, 9)
```
