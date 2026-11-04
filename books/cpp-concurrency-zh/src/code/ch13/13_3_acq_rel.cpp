// mode: run
// output: varies
// expect: 两个线程都没看到对方写的次数: [1-9][0-9]*
#include <atomic>
#include <barrier>
#include <cstdio>
#include <thread>

// 和 13_2 同一个 store buffer 形状，只把 store 换成 release、
// load 换成 acquire。acquire/release 只成对同步，没有全局总序。
std::atomic<bool> x{false}, y{false};
std::atomic<bool> ok_a{true}, ok_b{true};
std::atomic<int> z{0};
std::barrier start(3), done(3);
constexpr int kIters = 1'000'000;

void thread_a() {
    for (int i = 0; i < kIters; ++i) {
        start.arrive_and_wait();
        x.store(true, std::memory_order_release);
        ok_a.store(y.load(std::memory_order_acquire), std::memory_order_relaxed);
        done.arrive_and_wait();
    }
}

void thread_b() {
    for (int i = 0; i < kIters; ++i) {
        start.arrive_and_wait();
        y.store(true, std::memory_order_release);
        ok_b.store(x.load(std::memory_order_acquire), std::memory_order_relaxed);
        done.arrive_and_wait();
    }
}

int main() {
    std::thread a(thread_a), b(thread_b);
    for (int i = 0; i < kIters; ++i) {
        x.store(false, std::memory_order_relaxed);
        y.store(false, std::memory_order_relaxed);
        ok_a.store(true, std::memory_order_relaxed);
        ok_b.store(true, std::memory_order_relaxed);
        start.arrive_and_wait();
        done.arrive_and_wait();
        if (!ok_a.load(std::memory_order_relaxed) &&
            !ok_b.load(std::memory_order_relaxed)) {
            z.fetch_add(1, std::memory_order_relaxed);
        }
    }
    a.join();
    b.join();
    std::printf("两个线程都没看到对方写的次数: %d\n", z.load());
}
