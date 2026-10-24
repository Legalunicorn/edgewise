# V3: Persistent workers

- V3 creates workers once per BFS call and reuses them across layers.
- Two barriers coordinate each layer. The first prevents workers from scanning until the current frontier is ready. The second ensures that scanning has finished before the main thread combines output buffers and advances the frontier.
- Workers then wait at the next start barrier while the main thread prepares the next layer.
- V3 also checks whether a vertex is already visited before attempting compare-and-swap. Its comparison with V2 therefore includes both worker reuse and this discovery change.
- The main thread still assembles each next frontier sequentially.

