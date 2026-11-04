// mode: run
// expect: 校验: 通过
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

constexpr int kWorkers = 4;
constexpr int kThreshold = 1024;

std::vector<int> a;
std::mutex m;
std::condition_variable cv;
std::deque<std::pair<int, int>> tasks;   // 任务栈：[lo, hi)，从尾部进出
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
    // 三数取中，避免有序输入退化成 O(n^2)。
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
    // 左半留在当前线程继续拆，右半压栈交给空闲线程：LIFO，先取后压的右半。
    while (hi - lo >= kThreshold) {
        int p = partition_range(lo, hi);
        // 枢轴 a[p] 已在最终位置，两个子区间不重叠、也不遗漏。
        push(p + 1, hi);
        hi = p;
    }
    insertion_sort(lo, hi);
}

void worker() {
    for (;;) {
        std::pair<int, int> r;
        {
            std::unique_lock lock(m);
            cv.wait(lock, [] { return done || !tasks.empty(); });
            if (tasks.empty()) return;
            r = tasks.back();
            tasks.pop_back();
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
    constexpr int N = 1'000'000;
    a.resize(N);
    std::mt19937 rng(20261103);
    std::uniform_int_distribution<int> dist(-1'000'000, 1'000'000);
    uint64_t sum_before = 0, xor_before = 0;
    for (int& v : a) {
        v = dist(rng);
        sum_before += uint64_t(uint32_t(v));
        xor_before ^= uint64_t(uint32_t(v));
    }

    push(0, N);
    std::vector<std::thread> workers;
    for (int i = 0; i < kWorkers; ++i) workers.emplace_back(worker);
    {
        std::unique_lock lock(m);
        cv.wait(lock, [] { return done; });
    }
    for (auto& t : workers) t.join();

    uint64_t sum_after = 0, xor_after = 0;
    for (int v : a) {
        sum_after += uint64_t(uint32_t(v));
        xor_after ^= uint64_t(uint32_t(v));
    }
    bool sorted = std::is_sorted(a.begin(), a.end());
    bool same = sum_before == sum_after && xor_before == xor_after;
    std::printf("已排序: %s\n", sorted ? "是" : "否");
    std::printf("元素校验: %s（和 %llu，异或 %llu）\n", same ? "通过" : "失败",
                (unsigned long long)sum_after, (unsigned long long)xor_after);
    std::printf("校验: %s\n", sorted && same ? "通过" : "失败");
}
