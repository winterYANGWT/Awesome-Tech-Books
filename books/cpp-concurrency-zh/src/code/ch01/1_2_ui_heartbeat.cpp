// mode: run
// output: varies
// tsan: no（计时示例，TSan 会放大调度开销）
// expect: B 段最大间隔
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

using Clock = std::chrono::steady_clock;

std::atomic<std::uint64_t> ticks{0};
std::atomic<bool> stop{false};

// 心跳线程只负责按 50 ms 计一次数，不关心主线程有没有空处理。
void heartbeat() {
    while (!stop.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ticks.fetch_add(1, std::memory_order_relaxed);
    }
}

// 模拟界面事件循环：醒来时看看心跳计数有没有变，变了就记一次间隔。
void observe(std::uint64_t& last_tick, Clock::time_point& last_time,
             Clock::duration& max_gap) {
    const auto now = Clock::now();
    const auto t = ticks.load(std::memory_order_relaxed);
    if (t != last_tick) {
        const auto gap = now - last_time;
        if (gap > max_gap) max_gap = gap;
        last_tick = t;
        last_time = now;
    }
}

double busy_work(Clock::duration duration) {
    const auto end = Clock::now() + duration;
    double x = 0.0;
    std::uint64_t i = 0;
    while (Clock::now() < end) {
        for (int k = 0; k < 1000; ++k) {
            x += static_cast<double>((i + static_cast<std::uint64_t>(k)) % 97) * 0.001;
        }
        ++i;
    }
    return x;
}

int main() {
    // 预热，免得第一次调度和缺页把间隔拉偏。
    volatile double sink = busy_work(std::chrono::milliseconds(50));
    (void)sink;

    std::thread hb(heartbeat);

    std::uint64_t last_tick = 0;
    auto last_time = Clock::now();
    Clock::duration max_a{};

    // A 段：计算留在界面线程，800 ms 内不处理心跳。
    sink = busy_work(std::chrono::milliseconds(800));
    observe(last_tick, last_time, max_a);

    // B 段：计算交给工作线程，界面线程每 5 ms 观察一次。
    std::atomic<bool> done{false};
    std::thread work([&] {
        sink = busy_work(std::chrono::milliseconds(800));
        done.store(true, std::memory_order_relaxed);
    });

    Clock::duration max_b{};
    auto b_last = Clock::now();
    std::uint64_t b_tick = ticks.load(std::memory_order_relaxed);
    while (!done.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        observe(b_tick, b_last, max_b);
    }
    work.join();

    stop.store(true, std::memory_order_relaxed);
    hb.join();

    auto ms = [](Clock::duration d) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
    };
    std::cout << "A 段最大间隔（计算在主线程）：" << ms(max_a) << " ms\n";
    std::cout << "B 段最大间隔（计算在工作线程）：" << ms(max_b) << " ms\n";
    std::cout << "计算校验和：" << sink << "\n";
}
