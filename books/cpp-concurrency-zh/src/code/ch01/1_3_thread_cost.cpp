// mode: run
// output: varies
// tsan: no（计时示例，一次创建上千个线程）
// timeout: 60
// expect: ^threads,total_us,per_thread_us
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;

double measure(unsigned n) {
    const auto start = Clock::now();
    std::vector<std::thread> threads;
    threads.reserve(n);
    for (unsigned i = 0; i < n; ++i) threads.emplace_back([] {});
    for (auto& t : threads) t.join();
    const auto stop = Clock::now();
    return std::chrono::duration<double, std::micro>(stop - start).count();
}

int main() {
    // 预热，避开第一次建线程时的动态链接和页表开销。
    measure(8);

    std::cout << "threads,total_us,per_thread_us\n";
    for (unsigned n : {1u, 10u, 100u, 1000u}) {
        double best = 1e100;
        for (int rep = 0; rep < 3; ++rep) best = std::min(best, measure(n));
        std::cout << n << ',' << static_cast<std::uint64_t>(best) << ','
                  << static_cast<std::uint64_t>(best / n) << '\n';
    }
}
