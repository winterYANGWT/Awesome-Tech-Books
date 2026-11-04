// mode: run
// expect: z == 0 的次数: 0
// timeout: 60
#include <atomic>
#include <barrier>
#include <cstdio>
#include <thread>

// 两个写线程各写一个变量，两个读线程按相反的顺序读。
// 每轮开始前把 x、y 清掉，读线程先等自己的第一个变量为 true，
// 再看另一个变量；只有两个读线程都看到 false，才说明它们各看了一个世界。
std::atomic<bool> x{false}, y{false};
std::atomic<bool> ok_x{true}, ok_y{true};
std::atomic<int> z{0};
std::barrier start(5), done(5);
constexpr int kIters = 1'000'000;

void write_x() {
    for (int i = 0; i < kIters; ++i) {
        start.arrive_and_wait();
        x.store(true, std::memory_order_seq_cst);
        done.arrive_and_wait();
    }
}

void write_y() {
    for (int i = 0; i < kIters; ++i) {
        start.arrive_and_wait();
        y.store(true, std::memory_order_seq_cst);
        done.arrive_and_wait();
    }
}

void read_x_then_y() {
    for (int i = 0; i < kIters; ++i) {
        start.arrive_and_wait();
        while (!x.load(std::memory_order_seq_cst)) {
        }
        ok_x.store(y.load(std::memory_order_seq_cst), std::memory_order_relaxed);
        done.arrive_and_wait();
    }
}

void read_y_then_x() {
    for (int i = 0; i < kIters; ++i) {
        start.arrive_and_wait();
        while (!y.load(std::memory_order_seq_cst)) {
        }
        ok_y.store(x.load(std::memory_order_seq_cst), std::memory_order_relaxed);
        done.arrive_and_wait();
    }
}

int main() {
    std::thread wx(write_x), wy(write_y);
    std::thread rx(read_x_then_y), ry(read_y_then_x);
    for (int i = 0; i < kIters; ++i) {
        x.store(false, std::memory_order_relaxed);
        y.store(false, std::memory_order_relaxed);
        ok_x.store(true, std::memory_order_relaxed);
        ok_y.store(true, std::memory_order_relaxed);
        start.arrive_and_wait();
        done.arrive_and_wait();
        if (!ok_x.load(std::memory_order_relaxed) &&
            !ok_y.load(std::memory_order_relaxed)) {
            z.fetch_add(1, std::memory_order_relaxed);
        }
    }
    wx.join();
    wy.join();
    rx.join();
    ry.join();
    std::printf("z == 0 的次数: %d\n", z.load());
}
