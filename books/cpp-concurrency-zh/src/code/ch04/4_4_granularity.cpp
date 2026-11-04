// mode: run
// output: varies
// expect: 两把锁.*x != y [1-9]\d* 次
// 锁变细只在访问真的独立时有效：两个字段必须一起读时，两把锁反而更慢，
// 而且分开读两个字段会看到不一致的快照。
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;
constexpr int kIncrements = 1'000'000;
constexpr int kPairUpdates = 500'000;

double ms_since(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

int main() {
    // ANCHOR: independent
    alignas(64) std::uint64_t a = 0;  // 分开到不同缓存行，排除伪共享
    alignas(64) std::uint64_t b = 0;

    // 一把大锁：两个线程明明各加各的计数器，也要排队。
    std::mutex big;
    const auto t0 = Clock::now();
    std::thread t1([&] {
        for (int i = 0; i < kIncrements; ++i) {
            std::lock_guard<std::mutex> lock(big);
            ++a;
        }
    });
    std::thread t2([&] {
        for (int i = 0; i < kIncrements; ++i) {
            std::lock_guard<std::mutex> lock(big);
            ++b;
        }
    });
    t1.join();
    t2.join();
    const double big_ms = ms_since(t0);
    const std::uint64_t big_a = a;
    const std::uint64_t big_b = b;

    // 两把锁：各自保护一个独立的计数器，可以真正并行。
    std::mutex ma;
    std::mutex mb;
    a = 0;
    b = 0;
    const auto t3 = Clock::now();
    std::thread t4([&] {
        for (int i = 0; i < kIncrements; ++i) {
            std::lock_guard<std::mutex> lock(ma);
            ++a;
        }
    });
    std::thread t5([&] {
        for (int i = 0; i < kIncrements; ++i) {
            std::lock_guard<std::mutex> lock(mb);
            ++b;
        }
    });
    t4.join();
    t5.join();
    const double fine_ms = ms_since(t3);
    const std::uint64_t fine_a = a;
    const std::uint64_t fine_b = b;
    // ANCHOR_END: independent

    std::cout << "每个线程各加自己的计数器 " << kIncrements << " 次\n";
    std::cout << "一把大锁：" << big_ms << " ms，a=" << big_a << " b=" << big_b << '\n';
    std::cout << "两把锁：" << fine_ms << " ms，a=" << fine_a << " b=" << fine_b << '\n';

    // ANCHOR: pair
    // 两个字段必须一起变：x == y 是不变量。
    std::uint64_t x = 0;
    std::uint64_t y = 0;
    std::mutex pair_mutex;
    std::atomic<bool> done{false};
    std::uint64_t snapshots = 0;
    std::uint64_t inconsistent = 0;

    const auto t6 = Clock::now();
    std::thread w1([&] {
        for (int i = 0; i < kPairUpdates; ++i) {
            std::lock_guard<std::mutex> lock(pair_mutex);
            ++x;
            ++y;
        }
    });
    std::thread w2([&] {
        for (int i = 0; i < kPairUpdates; ++i) {
            std::lock_guard<std::mutex> lock(pair_mutex);
            ++x;
            ++y;
        }
    });
    std::thread reader([&] {
        while (!done.load(std::memory_order_relaxed)) {
            std::lock_guard<std::mutex> lock(pair_mutex);
            const std::uint64_t sx = x;
            const std::uint64_t sy = y;
            ++snapshots;
            if (sx != sy) {
                ++inconsistent;
            }
        }
    });
    w1.join();
    w2.join();
    done.store(true);
    reader.join();
    const double pair_big_ms = ms_since(t6);
    // ANCHOR_END: pair
    const std::uint64_t pair_big_snapshots = snapshots;
    const std::uint64_t pair_big_bad = inconsistent;

    // 两把锁分开读两个字段：写者仍要同时锁两把，读者却可以读到中间状态。
    x = 0;
    y = 0;
    std::mutex mx;
    std::mutex my;
    done.store(false);
    snapshots = 0;
    inconsistent = 0;

    const auto t7 = Clock::now();
    std::thread w3([&] {
        for (int i = 0; i < kPairUpdates; ++i) {
            std::scoped_lock lock(mx, my);
            ++x;
            ++y;
        }
    });
    std::thread w4([&] {
        for (int i = 0; i < kPairUpdates; ++i) {
            std::scoped_lock lock(mx, my);
            ++x;
            ++y;
        }
    });
    std::thread reader2([&] {
        while (!done.load(std::memory_order_relaxed)) {
            std::uint64_t sx;
            std::uint64_t sy;
            {
                std::lock_guard<std::mutex> lock(mx);
                sx = x;
            }
            {
                std::lock_guard<std::mutex> lock(my);
                sy = y;
            }
            ++snapshots;
            if (sx != sy) {
                ++inconsistent;
            }
        }
    });
    w3.join();
    w4.join();
    done.store(true);
    reader2.join();
    const double pair_fine_ms = ms_since(t7);

    std::cout << "两个字段一起更新（x == y 是不变量），写线程各更新 " << kPairUpdates << " 次\n";
    std::cout << "一把锁：" << pair_big_ms << " ms，快照 " << pair_big_snapshots
              << " 次，x != y " << pair_big_bad << " 次\n";
    std::cout << "两把锁：" << pair_fine_ms << " ms，快照 " << snapshots
              << " 次，x != y " << inconsistent << " 次\n";
}
