// mode: race
// expect: data race
#include <algorithm>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <mutex>
#include <random>
#include <thread>
#include <utility>
#include <vector>

constexpr int kWorkers = 4;
std::vector<int> a;
std::mutex m;
std::condition_variable cv;
std::deque<std::pair<int, int>> tasks;
bool done = false;
long pending = 0;

void push(int lo, int hi) {
    if (lo >= hi) return;
    std::lock_guard lock(m);
    tasks.emplace_back(lo, hi);
    ++pending;
    cv.notify_one();
}

int partition_range(int lo, int hi) {
    int mid = lo + (hi - lo) / 2;
    if (a[mid] < a[lo]) std::swap(a[mid], a[lo]);
    if (a[hi - 1] < a[lo]) std::swap(a[hi - 1], a[lo]);
    if (a[hi - 1] < a[mid]) std::swap(a[hi - 1], a[mid]);
    std::swap(a[mid], a[hi - 1]);
    int pivot = a[hi - 1];
    int i = lo;
    for (int j = lo; j < hi - 1; ++j)
        if (a[j] < pivot) std::swap(a[i++], a[j]);
    std::swap(a[i], a[hi - 1]);
    return i;
}

void insertion_sort(int lo, int hi) {
    for (int i = lo + 1; i < hi; ++i) {
        int v = a[i], j = i;
        while (j > lo && a[j - 1] > v) {
            a[j] = a[j - 1];
            --j;
        }
        a[j] = v;
    }
}

void sort_range(int lo, int hi) {
    if (hi - lo < 32) {
        insertion_sort(lo, hi);
        return;
    }
    int p = partition_range(lo, hi);
    if (p == lo) { push(lo + 1, hi); return; }
    if (p == hi - 1) { push(lo, hi - 1); return; }
    // 错误：两个区间都在 p 处闭合，a[p] 同时属于两边。
    push(lo, p + 1);
    push(p, hi);
}

void worker() {
    for (;;) {
        std::pair<int, int> r;
        {
            std::unique_lock lock(m);
            cv.wait(lock, [] { return done || !tasks.empty(); });
            if (tasks.empty()) return;
            r = tasks.front();
            tasks.pop_front();
        }
        sort_range(r.first, r.second);
        {
            std::lock_guard lock(m);
            if (--pending == 0) {
                done = true;
                cv.notify_all();
            }
        }
    }
}

int main() {
    constexpr int N = 200'000;
    a.resize(N);
    std::mt19937 rng(7);
    std::uniform_int_distribution<int> dist(0, 1'000'000);
    for (int& v : a) v = dist(rng);

    push(0, N);
    std::vector<std::thread> workers;
    for (int i = 0; i < kWorkers; ++i) workers.emplace_back(worker);
    {
        std::unique_lock lock(m);
        cv.wait(lock, [] { return done; });
    }
    for (auto& t : workers) t.join();
    std::printf("排序结束（结果是否正确不重要，边界已经重叠）\n");
}
