# Sequential DSU

::: info Under construction
This page is under construction.
:::

A disjoint-set union (DSU) maintains groups of elements. `find(v)` returns the representative of a group, and `union_set(a, b)` merges the groups containing `a` and `b`.

## Brief idea

Each element starts in its own group, with itself as its parent.

- `par[v]` stores the parent of `v`. A root is its own parent and represents the group.
- `find` follows parents to the root. Path compression updates the visited elements to point directly to that root.
- `union_set` attaches the smaller group to the larger group. This is union by size; group sizes are maintained at roots.

## Implementation

```cpp
#include <numeric>
#include <utility>
#include <vector>

struct SequentialDSU {
    int n;
    std::vector<int> par, size;

    explicit SequentialDSU(int sz) {
        n = sz;
        size.assign(n, 1);
        par.assign(n, 0);
        // Initially, every element is its own parent.
        std::iota(par.begin(), par.end(), 0);
    }

    int find(int v) {
        if (v == par[v]) return v;
        // Compress the path while returning the root.
        return par[v] = find(par[v]);
    }

    void union_set(int a, int b) {
        a = find(a);
        b = find(b);
        if (a != b) {
            // Keep a as the root of the larger group.
            if (size[a] < size[b]) std::swap(a, b);
            par[b] = a;
            size[a] += size[b];
        }
    }
};
```

With path compression and union by size, `find` and `union_set` take amortized `O(α(n))` time, where `α(n)` grows very slowly. Storage is `O(n)`.

See [Sequential CC using DSU](/cc/sequential-dsu) for its use on a graph, and [Concurrent DSU](/cc/dsu/concurrent) for the version shared by workers.
