# V2: Atomic discovery

- V2 replaces the shared discovery mutex with atomic distance entries.
- A compare-and-swap operation changes a vertex's distance from `-1` to the next layer's distance only if it is still undiscovered. Exactly one competing worker succeeds and records the vertex.
- Workers examining different vertices no longer pass through one shared mutex. Atomic operations still have costs, particularly when workers compete for the same destination.

Thread creation and joining still happen once per layer.

