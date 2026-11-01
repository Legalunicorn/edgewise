# Concurrent DSU

`ConcurrentDSU` lets multiple workers call `find` and `union_set` on the same DSU. It stores atomic parents.

This documentation extends from [Sequential DSU](/cc/dsu/sequential)

## Brief idea

Same as the sequential version, each vertex starts as its own parent. 
However, when joining components we no longer use a rank by size heuristic. Instead, the larger root ID points to the smaller root ID. This ensures that parent ID can only decrease and prevent accidental cycles when concurrent threads are updating the parents ID.


### `find` concurrent
1. first we follows parents to the **observed root**, then 
2. attempts to shorten that path using **compare-and-swap** (CAS). This uses the same conditional update idea as [BFS atomic discovery](/bfs/v2-atomic).


```cpp
int find(int v) {
    int root = v;
    // find observed root using atomic reads.
    while (true) {
        const int p = parent[root].load();
        if (p == root) break;
        root = p;
    }

    // path compression towards observed root
    while (true) {
        int p = parent[v].load();
        // stop if the parent already reaches that root or a smaller ID.
        if (p <= root) break;
        // A failed CAS updates p. continue from that observed parent.
        parent[v].compare_exchange_strong(p, root);
        v = p;
    }
    return root;
}
```

### `union` concurrent
Suppose we want to union `a` and `b`.

Let `rlow` and `rhigh` be the observed roots of `a` and `b`.
If `rlow` equals `rhigh` they belong to the same component, so we can return without merging them. Otherwise, swap them if necessary so that 'rlow < rhigh'.

Another thread may change either root’s parent after we observe it. Hence CAS checks that `rhigh` is still a root before attaching it. On the other hand, `rlow` may already point to a smaller parent but linking to it still connects the components.

Our intention is to now set `parent[rhigh] = rlow`.

For `parent[rhigh].compare_exchange_strong(rhigh, rlow)`

- If `parent[rhigh] == rhigh`, then `rhigh` is still its root, we can then change its parent to `rlow` and the operation is done.
- Otherwise, leave the parent unchanged, the roots has been updated by other threads after it was observed and before our CAS. Hence we can retry the union from the start again, and find the updated `rhigh` and `rlow`.

```cpp
void union_set(int a, int b) {
    while (true) {
        int rlow = find(a);
        int rhigh = find(b);
        if (rlow == rhigh) return;
        if (rlow > rhigh) std::swap(rlow, rhigh);
        // Attach the larger root only if it is still a root.
        if (parent[rhigh].compare_exchange_strong(rhigh, rlow)) return;
    }
}
```

While unions are running, a root returned by `find` may subsequently become a child. The union's CAS checks that the root being attached is still a root before updating it.

The parent array uses `O(V)` space.

See [Parallel CC using concurrent DSU](/cc/parallel-dsu) for how workers use this shared structure.

::: details Full implementation (reference)
```cpp
#include <atomic>
#include <utility>
#include <vector>

struct ConcurrentDSU {
    int n;
    std::vector<std::atomic<int>> parent;

    explicit ConcurrentDSU(int sz) : n(sz), parent(sz) {
        for (int i = 0; i < n; i++) parent[i].store(i);
    }

    int find(int v) {
        int root = v;
        // find root using atomic reads
        while (true) {
            const int p = parent[root].load();
            if (p == root) break;
            root = p;
        }

        // compres path towards the observed root.
        while (true) {
            int p = parent[v].load();
            // stop if the parent already reaches that root or a smaller ID.
            if (p <= root) break;
            // A failed CAS updates p; continue from that observed parent.
            parent[v].compare_exchange_strong(p, root);
            v = p;
        }
        return root;
    }

    void union_set(int a, int b) {
        while (true) {
            int rlow = find(a);
            int rhigh = find(b);
            if (rlow == rhigh) return;
            if (rlow > rhigh) std::swap(rlow, rhigh);
            // attach  the larger root only if it is still a root.
            if (parent[rhigh].compare_exchange_strong(rhigh, rlow)) return;
        }
    }
};
```
:::
