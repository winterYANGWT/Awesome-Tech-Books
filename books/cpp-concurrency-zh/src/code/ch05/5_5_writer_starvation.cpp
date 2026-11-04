// mode: run
// output: varies
// expect: 默认读者优先
#include <pthread.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

struct Shared {
    pthread_rwlock_t rw;
    long value = 0;
    std::atomic<bool> stop{false};
    std::atomic<int> ready{0};
};

static void reader(Shared* s) {
    s->ready.fetch_add(1, std::memory_order_relaxed);

    while (!s->stop.load(std::memory_order_relaxed)) {
        if (pthread_rwlock_rdlock(&s->rw) != 0) return;
        volatile long sink = s->value;  // 读锁保护下的读
        (void)sink;
        volatile long spin = 0;
        for (int i = 0; i < 2000; ++i) {
            spin = spin + i;  // 持读锁一小段时间，让多个读者的读锁重叠
        }
        (void)spin;
        pthread_rwlock_unlock(&s->rw);
    }
}

// kind 0：默认（glibc 上读者优先）；kind 1：写者优先（非递归读锁）。
static double measure(int readers, int kind) {
    Shared s;
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);
    if (kind == 1) {
        pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    }
    pthread_rwlock_init(&s.rw, &attr);

    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(readers));
    for (int i = 0; i < readers; ++i) threads.emplace_back(reader, &s);
    while (s.ready.load(std::memory_order_acquire) < readers) {
        std::this_thread::yield();
    }

    std::thread timer([&s] {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        s.stop.store(true, std::memory_order_relaxed);
    });

    double max_wait_ms = 0.0;
    while (!s.stop.load(std::memory_order_relaxed)) {
        auto start = std::chrono::steady_clock::now();
        if (pthread_rwlock_wrlock(&s.rw) != 0) break;
        auto got = std::chrono::steady_clock::now();
        s.value++;
        pthread_rwlock_unlock(&s.rw);

        std::chrono::duration<double, std::milli> wait = got - start;
        max_wait_ms = std::max(max_wait_ms, wait.count());
    }

    timer.join();
    for (std::thread& t : threads) t.join();
    pthread_rwlock_destroy(&s.rw);
    pthread_rwlockattr_destroy(&attr);
    return max_wait_ms;
}

int main() {
    std::printf("读者数,默认读者优先,写者优先\n");
    for (int n : {1, 2, 4, 8}) {
        double normal = measure(n, 0);
        double prefer_writer = measure(n, 1);
        std::printf("%d,%.3f,%.3f\n", n, normal, prefer_writer);
    }
}
