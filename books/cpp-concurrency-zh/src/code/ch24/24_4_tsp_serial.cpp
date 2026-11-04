// mode: run
// output: varies
// expect: recursive: best=
#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdio>
#include <vector>

constexpr int N = 12;       // 城市数
constexpr int MAXN = 16;    // 回路数组的固定容量
constexpr int INF = INT_MAX / 2;

int dist[N][N];

struct Tour {
    int city[MAXN];
    int n = 0;      // 已访问的城市数
    int cost = 0;   // 部分回路的代价
};

int best_cost = INF;
Tour best_tour;

// 和所有版本共用的图：有向，边权在 1..23 之间。
static void build_graph() {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            dist[i][j] = (i == j) ? 0 : ((i * 37 + j * 91 + 13) % 23) + 1;
        }
    }
}

static void consider_leaf(const Tour& t, long long& nodes) {
    ++nodes;
    int c = t.cost + dist[t.city[t.n - 1]][0];
    if (c < best_cost) {
        best_cost = c;
        best_tour = t;
        best_tour.cost = c;
    }
}

static void dfs(Tour& t, bool used[N], long long& nodes) {
    if (t.n == N) {
        consider_leaf(t, nodes);
        return;
    }
    ++nodes;
    int last = t.city[t.n - 1];
    for (int v = 0; v < N; ++v) {
        if (used[v]) continue;
        int c = t.cost + dist[last][v];
        if (c >= best_cost) continue;       // Feasible：代价已经追不上当前最优
        used[v] = true;
        t.city[t.n++] = v;
        t.cost = c;
        dfs(t, used, nodes);
        t.cost -= dist[last][v];
        --t.n;
        used[v] = false;
    }
}

static void run_recursive(long long& nodes, double& ms) {
    best_cost = INF;
    Tour root{};
    root.city[0] = 0;
    root.n = 1;
    bool used[N] = {};
    used[0] = true;
    nodes = 0;
    auto t0 = std::chrono::steady_clock::now();
    dfs(root, used, nodes);
    auto t1 = std::chrono::steady_clock::now();
    ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
}

// 栈里放整条部分回路：每个任务都能被单独取走，后面并行版用它。
static void run_iterative(long long& nodes, double& ms) {
    best_cost = INF;
    std::vector<Tour> stack;
    stack.reserve(1 << 16);
    Tour root{};
    root.city[0] = 0;
    root.n = 1;
    stack.push_back(root);
    nodes = 0;
    auto t0 = std::chrono::steady_clock::now();
    while (!stack.empty()) {
        Tour t = stack.back();
        stack.pop_back();
        if (t.cost >= best_cost) continue;  // 入栈后 best_cost 可能变小
        if (t.n == N) {
            consider_leaf(t, nodes);
            continue;
        }
        ++nodes;
        bool used[N] = {};
        for (int i = 0; i < t.n; ++i) used[t.city[i]] = true;
        int last = t.city[t.n - 1];
        // 倒序压栈，出栈顺序就和递归版的 v 从小到大一致。
        for (int v = N - 1; v >= 0; --v) {
            if (used[v]) continue;
            int c = t.cost + dist[last][v];
            if (c >= best_cost) continue;
            Tour child = t;
            child.city[child.n++] = v;
            child.cost = c;
            stack.push_back(child);
        }
    }
    auto t1 = std::chrono::steady_clock::now();
    ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
}

static double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

static void print_tour(const Tour& t) {
    for (int i = 0; i < t.n; ++i) std::printf("%d-", t.city[i]);
    std::printf("0");
}

int main() {
    build_graph();
    constexpr int REPS = 3;

    std::vector<double> rec_ms, iter_ms;
    long long rec_nodes = 0, iter_nodes = 0;
    int rec_best = 0, iter_best = 0;
    Tour rec_tour{}, iter_tour{};

    for (int r = 0; r < REPS; ++r) {
        double ms = 0;
        run_recursive(rec_nodes, ms);
        rec_ms.push_back(ms);
        rec_best = best_cost;
        rec_tour = best_tour;

        run_iterative(iter_nodes, ms);
        iter_ms.push_back(ms);
        iter_best = best_cost;
        iter_tour = best_tour;
    }

    std::printf("cities=%d\n", N);
    std::printf("recursive: best=%d tour=", rec_best);
    print_tour(rec_tour);
    std::printf(" nodes=%lld ms=%.3f\n", rec_nodes, median(rec_ms));
    std::printf("iterative: best=%d tour=", iter_best);
    print_tour(iter_tour);
    std::printf(" nodes=%lld ms=%.3f\n", iter_nodes, median(iter_ms));
    return 0;
}
