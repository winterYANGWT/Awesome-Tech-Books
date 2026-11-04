// mode: run
// output: varies
// tsan: no（计时示例，TSan 会放大线程创建开销）
// timeout: 60
// expect: 两种结果都已排序: 是
// expect: 并行任务数: [1-9][0-9]*
// 100 万个整数：串行 std::sort 对比带 cutoff 的 async 递归快排。
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
    std::nth_element(first, mid, last);

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
    constexpr int kN = 1000000;
    std::mt19937 gen(7);
    std::uniform_int_distribution<int> dist(0, 1000000);
    std::vector<int> base(kN);
    for (int& x : base) {
        x = dist(gen);
    }

    auto time_serial = [&] {
        std::vector<int> data = base;
        const auto t0 = std::chrono::steady_clock::now();
        std::sort(data.begin(), data.end());
        const auto t1 = std::chrono::steady_clock::now();
        return std::make_pair(
            std::chrono::duration<double, std::milli>(t1 - t0).count(),
            std::is_sorted(data.begin(), data.end()));
    };
    auto time_parallel = [&] {
        std::vector<int> data = base;
        tasks.store(0);
        const auto t0 = std::chrono::steady_clock::now();
        parallel_sort(data.data(), data.data() + data.size());
        const auto t1 = std::chrono::steady_clock::now();
        return std::make_pair(
            std::chrono::duration<double, std::milli>(t1 - t0).count(),
            std::is_sorted(data.begin(), data.end()));
    };

    (void)time_serial();    // 预热
    (void)time_parallel();

    std::vector<double> serial_times;
    std::vector<double> parallel_times;
    bool sorted = true;
    int last_tasks = 0;
    for (int round = 0; round < 3; ++round) {
        const auto serial = time_serial();
        const auto parallel = time_parallel();
        serial_times.push_back(serial.first);
        parallel_times.push_back(parallel.first);
        sorted = sorted && serial.second && parallel.second;
        last_tasks = tasks.load();
    }

    std::printf("元素数: %d\n", kN);
    std::printf("cutoff: %d\n", kCutoff);
    std::printf("串行 std::sort 中位数: %.2f ms\n", median(serial_times));
    std::printf("并行 async 快排中位数: %.2f ms\n", median(parallel_times));
    std::printf("并行任务数: %d\n", last_tasks);
    std::printf("两种结果都已排序: %s\n", sorted ? "是" : "否");
}
