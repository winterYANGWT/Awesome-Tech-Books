// mode: run
// output: varies
// expect: 99\.9
// tsan: no（计时示例；TSan 会改变锁的开销）
// 同一张表、同一组操作，只换锁和读比例；输出是纯 CSV，给 chart 读。
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;
constexpr int kThreads = 8;
constexpr int kOpsPerThread = 100'000;
constexpr int kKeys = 1024;

template <class Lock>
struct ReadGuard;
template <>
struct ReadGuard<std::mutex> {
    explicit ReadGuard(std::mutex& m) : guard(m) {}
    std::unique_lock<std::mutex> guard;
};
template <>
struct ReadGuard<std::shared_mutex> {
    explicit ReadGuard(std::shared_mutex& m) : guard(m) {}
    std::shared_lock<std::shared_mutex> guard;
};

template <class Lock>
double run_once(int read_permille, std::uint64_t& checksum_out) {
    std::map<int, int> table;
    for (int i = 0; i < kKeys; ++i) table[i] = i;
    Lock lock;
    std::atomic<std::uint64_t> checksum{0};

    const auto worker = [&](int tid) {
        std::uint64_t sum = 0;
        std::uint32_t x = 12345u + std::uint32_t(tid) * 7919u;
        for (int i = 0; i < kOpsPerThread; ++i) {
            x ^= x << 13;  // 每个线程有自己的伪随机序列
            x ^= x >> 17;
            x ^= x << 5;
            const int key = int(x % kKeys);
            if (i % 1000 < read_permille) {
                ReadGuard<Lock> guard(lock);  // 读：shared_mutex 用 shared_lock
                const auto it = table.find(key);
                sum += std::uint64_t(it->second);
            } else {
                std::unique_lock<Lock> guard(lock);  // 写：两种锁都独占
                table[key] = key + 1;
            }
        }
        checksum.fetch_add(sum, std::memory_order_relaxed);
    };

    const auto begin = Clock::now();
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) threads.emplace_back(worker, t);
    for (auto& thread : threads) thread.join();
    checksum_out = checksum.load(std::memory_order_relaxed);
    return std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
}

template <class Lock>
double median_ms(int read_permille, std::uint64_t& checksum) {
    (void)run_once<Lock>(read_permille, checksum);  // 预热
    std::vector<double> samples;
    for (int i = 0; i < 3; ++i) samples.push_back(run_once<Lock>(read_permille, checksum));
    std::sort(samples.begin(), samples.end());
    return samples[1];
}

int main() {
    volatile std::uint64_t sink = 0;  // 让结果参与可观察行为，防止整段被优化掉
    std::cout << "读比例（%）,std::mutex,std::shared_mutex\n";
    for (int permille : {500, 900, 999}) {
        std::uint64_t c1 = 0;
        std::uint64_t c2 = 0;
        const double mutex_ms = median_ms<std::mutex>(permille, c1);
        const double shared_ms = median_ms<std::shared_mutex>(permille, c2);
        sink = c1 + c2 + std::uint64_t(mutex_ms * 1000) + std::uint64_t(shared_ms * 1000);
        std::cout << (permille / 10.0) << ',' << mutex_ms << ',' << shared_ms << '\n';
    }
    return sink == 0 ? 1 : 0;
}
