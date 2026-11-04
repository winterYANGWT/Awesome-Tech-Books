// mode: run
// output: varies
// expect: 8,
// 为了稳定复现“读者不断插队”，让读者按 1 ms 的间隔依次到达；
// 每个读者持有读锁 1.2 ms，写者在第一个读者进入后请求写锁。
#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;

int main() {
    std::cout << "读者数,写者最大等待毫秒\n";
    for (int readers : {1, 2, 4, 8}) {
        double worst = 0;
        for (int round = 0; round < 3; ++round) {
            std::shared_mutex lock;
            std::vector<std::thread> threads;
            for (int i = 0; i < readers; ++i) {
                threads.emplace_back([&, i] {
                    std::this_thread::sleep_for(i * 1ms);
                    std::shared_lock<std::shared_mutex> guard(lock);
                    std::this_thread::sleep_for(1200us);
                });
            }
            std::this_thread::sleep_for(500us);
            const auto begin = Clock::now();
            {
                std::unique_lock<std::shared_mutex> guard(lock);
                const double wait =
                    std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
                worst = std::max(worst, wait);
            }
            for (auto& thread : threads) thread.join();
        }
        std::cout << readers << ',' << worst << '\n';
    }
}
