// mode: run
// output: varies
// expect: static: best=
#include <algorithm>
#include <atomic>
#include <chrono>
#include <climits>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

constexpr int N = 12;
constexpr int MAXN = 16;
constexpr int INF = INT_MAX / 2;
constexpr int THREADS = 4;
constexpr int REPS = 20;   // 单次搜索很短，重复取平均

int dist[N][N];

struct Tour {
    int city[MAXN];
    int n = 0;
    int cost = 0;
};

std::atomic<int> best_cost{INF};
std::mutex best_mutex;
Tour best_tour;
long long nodes[THREADS];

static void build_graph() {
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            dist[i][j] = (i == j) ? 0 : ((i * 37 + j * 91 + 13) % 23) + 1;
}

// 广度优先展开，直到至少 want 条部分回路。
static std::vector<Tour> partition_tree(int want) {
    std::vector<Tour> level;
    Tour root{};
    root.city[0] = 0;
    root.n = 1;
    level.push_back(root);
    while (static_cast<int>(level.size()) < want) {
        std::vector<Tour> next;
        for (const Tour& t : level) {
            bool used[N] = {};
            for (int i = 0; i < t.n; ++i) used[t.city[i]] = true;
            int last = t.city[t.n - 1];
            for (int v = 0; v < N; ++v) {
                if (used[v]) continue;
                Tour child = t;
                child.city[child.n++] = v;
                child.cost = t.cost + dist[last][v];
                next.push_back(child);
            }
        }
        if (next.empty()) break;
        level.swap(next);
    }
    return level;
}

static void try_update(int c, const Tour& t) {
    if (c >= best_cost.load(std::memory_order_relaxed)) return;
    std::lock_guard<std::mutex> lk(best_mutex);
    if (c < best_cost.load(std::memory_order_relaxed)) {
        best_tour = t;
        best_tour.cost = c;
        best_cost.store(c, std::memory_order_release);
    }
}

static void search(int id, std::vector<Tour> seed) {
    std::vector<Tour> stack;
    stack.reserve(1024);
    for (Tour& t : seed) stack.push_back(std::move(t));

    long long local = 0;
    while (!stack.empty()) {
        Tour t = stack.back();
        stack.pop_back();
        if (t.cost >= best_cost.load(std::memory_order_relaxed)) continue;
        if (t.n == N) {
            ++local;
            try_update(t.cost + dist[t.city[t.n - 1]][0], t);
            continue;
        }
        ++local;
        bool used[N] = {};
        for (int i = 0; i < t.n; ++i) used[t.city[i]] = true;
        int last = t.city[t.n - 1];
        for (int v = N - 1; v >= 0; --v) {
            if (used[v]) continue;
            int c = t.cost + dist[last][v];
            if (c >= best_cost.load(std::memory_order_relaxed)) continue;
            Tour child = t;
            child.city[child.n++] = v;
            child.cost = c;
            stack.push_back(child);
        }
    }
    nodes[id] = local;
}

int main() {
    build_graph();
    best_cost.store(INF);
    std::vector<Tour> parts = partition_tree(THREADS);
    std::printf("static: cities=%d threads=%d initial_tours=%zu\n",
                N, THREADS, parts.size());

    long long total_nodes[THREADS] = {};
    int final_best = INF;
    double total_ms = 0;
    for (int rep = 0; rep < REPS; ++rep) {
        best_cost.store(INF);
        auto t0 = std::chrono::steady_clock::now();
        std::vector<std::thread> threads;
        for (int id = 0; id < THREADS; ++id) {
            // 连续块：11 条回路按 3,3,3,2 分给四个线程。
            int lo = static_cast<int>(parts.size()) * id / THREADS;
            int hi = static_cast<int>(parts.size()) * (id + 1) / THREADS;
            std::vector<Tour> seed(parts.begin() + lo, parts.begin() + hi);
            threads.emplace_back(search, id, std::move(seed));
        }
        for (std::thread& th : threads) th.join();
        auto t1 = std::chrono::steady_clock::now();
        total_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();
        for (int id = 0; id < THREADS; ++id) total_nodes[id] += nodes[id];
        final_best = best_cost.load();
    }

    std::printf("static: best=%d nodes=", final_best);
    long long total = 0;
    for (int id = 0; id < THREADS; ++id) {
        std::printf("%lld%s", total_nodes[id], id + 1 == THREADS ? "" : ",");
        total += total_nodes[id];
    }
    std::printf(" total=%lld ms_per_rep=%.3f\n", total, total_ms / REPS);
    return 0;
}
