#include <iostream>
#include <vector>
#include <queue>



// represents a graph 
// responsibile for all graph functions
struct Graph{
    // graph stored as adjacency list
    std::vector<std::vector<int>> adj_list;
    std::vector<bool> seen;
    size_t size;
    Graph(size_t n){
        adj_list.resize(n);
        seen.resize(n);
        size = n;
    }

    // add a direced edged from u to v
    // 1-indexed
    void add_edge(int u, int v){
        adj_list[u].push_back(v);
    }

    // perform a bfs starting at source s
    // return a list of visited vertices and distaince as pair<int,int> each {vertice_id, depth}
    //
    std::vector<std::pair<int,int>> bfs(int s){
        std::vector<std::pair<int,int>> result;
        if (s < 0 || s >= size) {
            std::cerr << "Source s must be >= 0 and < Size of graph";
            return result;
        }

        seen.assign(size, false);
        std::queue<std::pair<int,int>> frontier;
        frontier.push({s,0});
        result.push_back({s + 1, 0});
        seen[s] = true;
        while(frontier.size() > 0) {
            int len = frontier.size();
            for (size_t i = 0; i < len; i++){
                auto [u, d] = frontier.front();
                frontier.pop();
                for (auto& v: adj_list[u]){
                    if (seen[v]) continue;
                    seen[v] = true;
                    frontier.push({v, d+1});
                    // output as one index
                    result.push_back({v+1, d+1});
                }
            }
        }
        return result;
    };
};


int main(){
    // input is one formated
    // reading a undirected graph with n vertices, m edged
    // input format: n,m, 
    // m lines: u,v edge from vertice u to v
    int n,m;
    std::cin >> n >> m;
    Graph g(n);
    for (int i = 0; i < m; i++){
        int u, v;
        std::cin >> u >> v;
        u--;
        v--;
        g.add_edge(u, v);
        g.add_edge(v, u);
    }
    auto bfs = g.bfs(0);
    for (auto& [v, d]: bfs) {
        std::cout << v << " "  << d << "\n";
    }
}

