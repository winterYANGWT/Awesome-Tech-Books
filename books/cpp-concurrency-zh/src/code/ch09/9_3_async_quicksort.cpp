// mode: run
// output: varies
// tsan: no（计时示例，TSan 会放大线程创建开销）
// timeout: 30
// expect: 已排序: 是
// expect: 任务数: [1-9][0-9]*
// 递归快排：小于 4096 个元素就串行，否则左半区交给 async。
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <future>
#include <random>
#include <vector>

namespace {

constexpr int kCutoff = 4096;
std::atomic<int> tasks{0};

void parallel_sort(int* first, int* last) {
    if (last - first < kCutoff) {
        std::sort(first, last);
        return;
    }
    int* mid = first + (last - first) / 2;
    std::nth_element(first, mid, last);  // mid 归位，左小右大。

    tasks.fetch_add(1, std::memory_order_relaxed);
    std::future<void> left =
        std::async(std::launch::async, [first, mid] { parallel_sort(first, mid); });
    parallel_sort(mid, last);
    left.get();
}

double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

}  // namespace

int main() {
    constexpr int kN = 200000;
    std::mt19937 gen(42);
    std::uniform_int_distribution<int> dist(0, 1000000);
    std::vector<int> base(kN);
    for (int& x : base) {
        x = dist(gen);
    }

    // 预热一次，避免第一次的线程创建和页错误混进中位数。
    {
        std::vector<int> warm = base;
        tasks.store(0);
        parallel_sort(warm.data(), warm.data() + warm.size());
    }

    std::vector<double> times;
    int last_tasks = 0;
    bool sorted = true;
    for (int round = 0; round < 3; ++round) {
        std::vector<int> data = base;
        tasks.store(0);
        const auto t0 = std::chrono::steady_clock::now();
        parallel_sort(data.data(), data.data() + data.size());
        const auto t1 = std::chrono::steady_clock::now();
        times.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
        last_tasks = tasks.load();
        sorted = sorted && std::is_sorted(data.begin(), data.end());
    }

    std::printf("元素数: %d\n", kN);
    std::printf("cutoff: %d\n", kCutoff);
    std::printf("任务数: %d\n", last_tasks);
    std::printf("已排序: %s\n", sorted ? "是" : "否");
    std::printf("耗时中位数: %.2f ms\n", median(times));
}
