#pragma once
#include <vector>
#include <utility>
#include <atomic>


// concurrent DSU 
// we need a strict ordering, by convention the lower ID wins
struct ConcurrentDSU{
public:
    int n; 
    std::vector<std::atomic<int>> parent;
    explicit ConcurrentDSU(int sz) : n(sz), parent(sz) {
        for (int i  = 0; i < n; i++) parent[i].store(i);
    }

    int find(int v){
        int root = v;
        // find root using read only
        while (true){
            const int p = parent[root].load();
            if (p==root) break;
            root = p;
        }

        // shorten path towards root
        while(true){
            int p = parent[v].load();
            // parent is already our root (=) OR 
            // parent has moved to a smaller ID due to another thread
            if (p <= root) break;
            // a failed CAS updates p; continue from that observed parent.
            parent[v].compare_exchange_strong(p, root);
            v = p;
        }
        return root;
    }

    void union_set(int a, int b){
        while(true) {
            int pa = find(a);
            int pb = find(b);
            if (pa == pb) return;
            if (pa > pb) std::swap(pa, pb);
            // if pb is still a root, make pa its parent.
            if (parent[pb].compare_exchange_strong(pb, pa)) return;
            // retry..
        }
    }
};

