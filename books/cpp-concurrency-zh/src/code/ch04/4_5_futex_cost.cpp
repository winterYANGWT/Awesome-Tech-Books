// mode: run
// output: varies
// expect: 有竞争
// 无竞争时加解锁只在用户态改一个原子变量；有竞争时才会进内核等待。
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;
constexpr int kSingleOps = 5'000'000;
constexpr int kThreads = 8;
constexpr int kPerThread = 200'000;

double uncontended_once(std::mutex& m, std::uint64_t& counter) {
    const auto t0 = Clock::now();
    for (int i = 0; i < kSingleOps; ++i) {
        std::lock_guard<std::mutex> lock(m);
        ++counter;
    }
    const double ns = std::chrono::duration<double, std::nano>(Clock::now() - t0).count();
    return ns / kSingleOps;
}

double contended_once(std::mutex& m, std::uint64_t& counter) {
    const auto t0 = Clock::now();
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < kPerThread; ++i) {
                std::lock_guard<std::mutex> lock(m);
                ++counter;
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    const double ns = std::chrono::duration<double, std::nano>(Clock::now() - t0).count();
    return ns / (double(kThreads) * kPerThread);
}

int main() {
    std::mutex m;
    std::uint64_t counter = 0;
    std::vector<double> single;
    std::vector<double> contended;

    for (int i = 0; i < 3; ++i) {
        single.push_back(uncontended_once(m, counter));
        contended.push_back(contended_once(m, counter));
    }
    std::sort(single.begin(), single.end());
    std::sort(contended.begin(), contended.end());

    std::cout << "无竞争：1 个线程加解锁 " << kSingleOps << " 次，每次 " << single[1] << " ns\n";
    std::cout << "有竞争：" << kThreads << " 个线程各加解锁 " << kPerThread << " 次，每次 "
              << contended[1] << " ns\n";
    std::cout << "counter = " << counter << '\n';
}
