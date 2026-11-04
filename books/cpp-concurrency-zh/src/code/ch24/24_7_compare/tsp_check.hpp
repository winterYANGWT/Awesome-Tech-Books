#pragma once
#include <climits>
#include <vector>

namespace tspcheck {

constexpr int N = 12;
constexpr int MAXN = 16;
constexpr int INF = INT_MAX / 2;

struct Tour {
    int city[MAXN];
    int n = 0;
    int cost = 0;
};

inline int best_cost_of_graph() {
    int dist[N][N];
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            dist[i][j] = (i == j) ? 0 : ((i * 37 + j * 91 + 13) % 23) + 1;

    int best = INF;
    std::vector<Tour> stack;
    Tour root{};
    root.city[0] = 0;
    root.n = 1;
    stack.push_back(root);
    while (!stack.empty()) {
        Tour t = stack.back();
        stack.pop_back();
        if (t.cost >= best) continue;
        if (t.n == N) {
            int c = t.cost + dist[t.city[t.n - 1]][0];
            if (c < best) best = c;
            continue;
        }
        bool used[N] = {};
        for (int i = 0; i < t.n; ++i) used[t.city[i]] = true;
        int last = t.city[t.n - 1];
        for (int v = N - 1; v >= 0; --v) {
            if (used[v]) continue;
            int c = t.cost + dist[last][v];
            if (c >= best) continue;
            Tour child = t;
            child.city[child.n++] = v;
            child.cost = c;
            stack.push_back(child);
        }
    }
    return best;
}

}  // namespace tspcheck
