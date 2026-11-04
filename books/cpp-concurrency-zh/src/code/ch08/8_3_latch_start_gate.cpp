// mode: run
// output: varies
// expect: 起跑前进入工作区的线程数: 0
// 8 个线程各自准备，ready 计数到 0 后主线程打开 go，全部同时起跑。
#include <latch>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

constexpr int kThreads = 8;
using Clock = std::chrono::steady_clock;

std::latch ready{kThreads};
std::latch go{1};
std::atomic<long long> gate_time_ns{0};
long long entry_ns[kThreads];

long long since_start(Clock::time_point start) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
}

int main() {
    auto start = Clock::now();
    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([i, start] {
            // 准备阶段：每个线程花的时间不同
            std::this_thread::sleep_for(std::chrono::microseconds(800 + 100 * i));
            ready.count_down();
            go.wait();  // 最后一个 count_down 之后，所有线程一起离开这里
            entry_ns[i] = since_start(start);
        });
    }
    ready.wait();                       // 等 8 个线程全部就绪
    gate_time_ns = since_start(start);  // 记录打开起跑线的时刻
    go.count_down();

    for (auto& t : threads) t.join();

    long long first = *std::min_element(entry_ns, entry_ns + kThreads);
    long long last = *std::max_element(entry_ns, entry_ns + kThreads);
    int early = 0;
    for (long long t : entry_ns) {
        if (t < gate_time_ns) ++early;
    }
    std::printf("全部就绪用时 %.2f ms\n", gate_time_ns / 1e6);
    std::printf("起跑后进入工作区的时间跨度 %.1f us\n", (last - first) / 1e3);
    std::printf("起跑前进入工作区的线程数: %d\n", early);
}
