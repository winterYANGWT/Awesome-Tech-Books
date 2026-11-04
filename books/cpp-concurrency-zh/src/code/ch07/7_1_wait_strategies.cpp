// mode: run
// output: varies
// tsan: no（计时示例，TSan 会改变等待和 CPU 时间）
// expect: 忙轮询,\d+\.\d+,\d+\.\d+\n1 ms 睡眠,\d+\.\d+,\d+\.\d+\n条件变量,\d+\.\d+,\d+\.\d+
// 用三种办法等同一个标志：忙轮询、每 1 ms 看一次、条件变量。
// 每种策略重复 100 次，取等待时间和 CPU 时间的中位数，输出 CSV 给 chart 用。
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace {

constexpr int kRounds = 100;
constexpr auto kDelay = 200us;

struct Sample {
    double wait_us;
    double cpu_us;
};

double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

// ANCHOR: wait
// 生产者睡 kDelay 后把 ready 置真；三种策略都在这个时间窗里等待。
// 0 = 忙轮询，1 = 每 1 ms 看一次，2 = 条件变量。
Sample wait_once(int strategy) {
    std::atomic<bool> ready{false};
    std::mutex m;
    std::condition_variable cv;

    const auto wall0 = std::chrono::steady_clock::now();
    const auto cpu0 = std::clock();

    std::thread producer([&] {
        std::this_thread::sleep_for(kDelay);
        if (strategy == 2) {
            {
                std::lock_guard<std::mutex> lock(m);
                ready.store(true, std::memory_order_release);
            }
            cv.notify_one();
        } else {
            ready.store(true, std::memory_order_release);
        }
    });

    if (strategy == 0) {
        while (!ready.load(std::memory_order_acquire)) {
        }
    } else if (strategy == 1) {
        while (!ready.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(1ms);
        }
    } else {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [&] { return ready.load(std::memory_order_acquire); });
    }

    producer.join();
    const auto wall1 = std::chrono::steady_clock::now();
    const auto cpu1 = std::clock();

    const double wall_us =
        std::chrono::duration<double, std::micro>(wall1 - wall0).count();
    const double cpu_us = 1e6 * double(cpu1 - cpu0) / CLOCKS_PER_SEC;
    return {wall_us, cpu_us};
}
// ANCHOR_END: wait

Sample measure(const char* name, int strategy) {
    // 预热一次，避免第一次的线程创建和页错误混进中位数。
    (void)wait_once(strategy);
    std::vector<double> waits;
    std::vector<double> cpus;
    for (int i = 0; i < kRounds; ++i) {
        const Sample s = wait_once(strategy);
        waits.push_back(s.wait_us);
        cpus.push_back(s.cpu_us);
    }
    const Sample result{median(waits), median(cpus)};
    std::printf("%s,%.1f,%.1f\n", name, result.wait_us, result.cpu_us);
    return result;
}

}  // namespace

int main() {
    std::printf("策略,平均等待微秒,CPU 微秒\n");
    const Sample busy = measure("忙轮询", 0);
    const Sample sleepy = measure("1 ms 睡眠", 1);
    const Sample cond = measure("条件变量", 2);
    // 结果参与一个可见的判断，防止整个测量被优化掉。
    if (busy.wait_us <= 0.0 || sleepy.wait_us <= 0.0 || cond.wait_us <= 0.0) {
        return 1;
    }
    return 0;
}
