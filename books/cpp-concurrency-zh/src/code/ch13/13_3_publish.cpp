// mode: run
// expect: 读到不一致配置的次数: 0
#include <atomic>
#include <cstdio>
#include <thread>

struct Config {
    int workers;
    int queue_size;
    long timeout_ms;
};

Config config{};
std::atomic<bool> ready{false};
std::atomic<bool> done{true};
constexpr int kIters = 100'000;

void writer() {
    for (int i = 0; i < kIters; ++i) {
        while (!done.load(std::memory_order_acquire)) {
        }
        done.store(false, std::memory_order_relaxed);
        config = Config{i + 1, 2 * (i + 1), 3L * (i + 1)};
        ready.store(true, std::memory_order_release);
    }
}

void reader() {
    int bad = 0;
    for (int i = 0; i < kIters; ++i) {
        while (!ready.load(std::memory_order_acquire)) {
        }
        Config snapshot = config;
        if (snapshot.queue_size != 2 * snapshot.workers ||
            snapshot.timeout_ms != 3L * snapshot.workers) {
            ++bad;
        }
        ready.store(false, std::memory_order_relaxed);
        done.store(true, std::memory_order_release);
    }
    std::printf("读到不一致配置的次数: %d\n", bad);
}

int main() {
    std::thread w(writer), r(reader);
    w.join();
    r.join();
}
