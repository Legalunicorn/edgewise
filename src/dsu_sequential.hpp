#pragma once
#include <vector>
#include <numeric>
#include <utility>

struct SequentialDSU{
public:
    int n; 
    std::vector<int> par, size;
    explicit SequentialDSU(int sz){
        n = sz;
        size.assign(n,1);
        par.assign(n,0);
        std::iota(par.begin(),par.end(),0);
    }

    int find(int v){
        if (v == par[v]) return v;
        return par[v] = find(par[v]);
    }

    void union_set(int a, int b){
        a = find(a);
        b = find(b);
        if (a != b){
            if (size[a] < size[b]) std::swap(a,b);
            par[b] = a;
            size [a] += size[b];
        }
    }
};

