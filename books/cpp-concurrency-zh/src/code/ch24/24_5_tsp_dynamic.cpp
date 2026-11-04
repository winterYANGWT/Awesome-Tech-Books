// mode: run
// output: varies
// expect: dynamic: best=60
#include <algorithm>
#include <atomic>
#include <chrono>
#include <climits>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

constexpr int N = 12;
constexpr int MAXN = 16;
constexpr int INF = INT_MAX / 2;
constexpr int THREADS = 4;
constexpr int CUTOFF = 9;   // 已访问 9 个城市的部分回路不再拆分
constexpr int REPS = 20;    // 单次搜索很短，重复取平均

int dist[N][N];

struct Tour {
    int city[MAXN];
    int n = 0;
    int cost = 0;
};

std::atomic<int> best_cost{INF};
std::mutex best_mutex;
Tour best_tour;

std::mutex term_mutex;
std::condition_variable term_cv;
std::vector<Tour> handoff;          // 待交接的 new_stack
int idle = 0;                       // 正在等待工作的线程数
int active = THREADS;               // 还没退出的线程数
bool finished = false;
std::atomic<int> waiting_hint{0};   // idle 的无锁快照，只用于快速检查
std::atomic<int> split_count{0};

long long nodes[THREADS];

static void build_graph() {
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            dist[i][j] = (i == j) ? 0 : ((i * 37 + j * 91 + 13) % 23) + 1;
}

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

// 调用者持有 term_mutex。
static bool split_locked(std::vector<Tour>& stack) {
    std::vector<Tour> eligible, rest;
    eligible.reserve(stack.size());
    for (Tour& t : stack) {
        if (t.n < CUTOFF) eligible.push_back(std::move(t));
        else rest.push_back(std::move(t));
    }
    if (eligible.size() < 2) {
        stack.clear();
        for (Tour& t : eligible) stack.push_back(std::move(t));
        for (Tour& t : rest) stack.push_back(std::move(t));
        return false;
    }
    std::sort(eligible.begin(), eligible.end(),
              [](const Tour& a, const Tour& b) { return a.n < b.n; });
    std::vector<Tour> keep, give;
    keep.reserve(eligible.size());
    give.reserve(eligible.size() / 2 + 1);
    for (size_t i = 0; i < eligible.size(); ++i) {
        if (i % 2 == 0) keep.push_back(std::move(eligible[i]));
        else give.push_back(std::move(eligible[i]));
    }
    stack.clear();
    for (Tour& t : keep) stack.push_back(std::move(t));
    for (Tour& t : rest) stack.push_back(std::move(t));
    handoff = std::move(give);
    ++split_count;
    term_cv.notify_one();
    return true;
}

static bool try_split(std::vector<Tour>& stack) {
    if (stack.size() < 2 || waiting_hint.load(std::memory_order_relaxed) == 0)
        return false;
    std::lock_guard<std::mutex> lk(term_mutex);
    // 快速检查之后条件可能已经变了，所以持锁再检查一遍。
    if (!handoff.empty() || waiting_hint.load(std::memory_order_relaxed) == 0 ||
        stack.size() < 2)
        return false;
    return split_locked(stack);
}

static void search(int id, std::vector<Tour> seed) {
    std::vector<Tour> stack = std::move(seed);
    long long local = 0;
    for (;;) {
        if (stack.empty()) {
            std::unique_lock<std::mutex> lk(term_mutex);
            ++idle;
            waiting_hint.store(idle, std::memory_order_relaxed);
            while (handoff.empty() && !finished) {
                if (idle == active) {
                    // 所有人都空闲、也没有待交接的栈：由最后一个线程宣布结束。
                    finished = true;
                    term_cv.notify_all();
                    break;
                }
                term_cv.wait(lk);
            }
            if (!handoff.empty()) {
                stack = std::move(handoff);
                handoff.clear();
            }
            --idle;
            waiting_hint.store(idle, std::memory_order_relaxed);
            if (finished && stack.empty()) {
                --active;
                term_cv.notify_all();
                nodes[id] = local;
                return;
            }
            lk.unlock();
        }
        if (stack.empty()) continue;

        while (!stack.empty()) {
            if (stack.size() >= 2 && try_split(stack)) break;
            Tour t = std::move(stack.back());
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
    }
}

int main() {
    build_graph();
    std::vector<Tour> parts = partition_tree(THREADS);
    std::printf("dynamic: cities=%d threads=%d cutoff=%d reps=%d\n",
                N, THREADS, CUTOFF, REPS);

    double total_ms = 0;
    long long total_nodes[THREADS] = {};
    int total_splits = 0;
    int final_best = INF;
    for (int rep = 0; rep < REPS; ++rep) {
        best_cost.store(INF);
        split_count.store(0);
        idle = 0;
        active = THREADS;
        finished = false;
        waiting_hint.store(0);
        handoff.clear();

        auto t0 = std::chrono::steady_clock::now();
        std::vector<std::thread> threads;
        for (int id = 0; id < THREADS; ++id) {
            std::vector<Tour> seed;
            if (id == 0) seed = parts;
            threads.emplace_back(search, id, std::move(seed));
        }
        for (std::thread& th : threads) th.join();
        auto t1 = std::chrono::steady_clock::now();
        total_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();
        for (int id = 0; id < THREADS; ++id) total_nodes[id] += nodes[id];
        total_splits += split_count.load();
        final_best = best_cost.load();
    }

    std::printf("dynamic: best=%d splits=%d nodes=", final_best, total_splits);
    long long sum = 0;
    for (int id = 0; id < THREADS; ++id) {
        std::printf("%lld%s", total_nodes[id], id + 1 == THREADS ? "" : ",");
        sum += total_nodes[id];
    }
    std::printf(" total=%lld ms_per_rep=%.3f\n", sum, total_ms / REPS);
    return 0;
}
