// mode: run
// output: varies
// expect: 校验失败 0 次
// tsan: no（TSan 不支持 atomic_thread_fence，会误报）
#include <atomic>
#include <barrier>
#include <cstdio>
#include <thread>

// seqlock：写者用奇数/偶数版本号标出“正在写”，读者发现版本变了就重试。
// 字段用 relaxed 原子访问，避免 C++ 层面的数据竞争；两个字段合起来仍不是原子的。
class SeqLock {
public:
    void write(int value) {
        unsigned s = seq_.load(std::memory_order_relaxed);
        seq_.store(s + 1, std::memory_order_relaxed);     // 奇数：开始写
        std::atomic_thread_fence(std::memory_order_release);
        x_.store(value, std::memory_order_relaxed);
        y_.store(value * 2, std::memory_order_relaxed);
        seq_.store(s + 2, std::memory_order_release);     // 偶数：写完
    }

    void read(int& x, int& y, long& retries) {
        for (;;) {
            unsigned before = seq_.load(std::memory_order_acquire);
            if (before & 1U) {                            // 写者正在写
                ++retries;
                continue;
            }
            x = x_.load(std::memory_order_relaxed);
            y = y_.load(std::memory_order_relaxed);
            std::atomic_thread_fence(std::memory_order_acquire);
            unsigned after = seq_.load(std::memory_order_relaxed);
            if (before == after) {
                return;
            }
            ++retries;
        }
    }

private:
    std::atomic<unsigned> seq_{0};
    std::atomic<int> x_{0}, y_{0};
};

constexpr int kUpdates = 1'000'000;
SeqLock lock;
std::atomic<bool> done{false};
std::atomic<long> retries{0}, failures{0}, reads{0};
std::barrier start(5);

void writer() {
    start.arrive_and_wait();
    for (int i = 0; i < kUpdates; ++i) {
        lock.write(i);
    }
    done.store(true, std::memory_order_release);
}

void reader() {
    long local_retries = 0, local_reads = 0, local_failures = 0;
    start.arrive_and_wait();
    while (!done.load(std::memory_order_acquire)) {
        int x = 0, y = 0;
        lock.read(x, y, local_retries);
        ++local_reads;
        // 写者始终写 y == x * 2；混合了两次写的值就会破坏它。
        if (y != x * 2) {
            ++local_failures;
        }
    }
    retries.fetch_add(local_retries, std::memory_order_relaxed);
    reads.fetch_add(local_reads, std::memory_order_relaxed);
    failures.fetch_add(local_failures, std::memory_order_relaxed);
}

int main() {
    std::thread w(writer);
    std::thread r1(reader), r2(reader), r3(reader), r4(reader);
    w.join();
    r1.join();
    r2.join();
    r3.join();
    r4.join();
    std::printf("校验失败 %ld 次，重试 %ld 次，读取 %ld 次\n",
                failures.load(), retries.load(), reads.load());
}
