// mode: hang
// expect: main: 两个线程已启动，等它们结束
// expect: 仍未结束
// 用 latch 把两个线程都推到“持有一把锁”的位置，再让它们去拿对方那把。
#include <iostream>
#include <latch>
#include <mutex>
#include <thread>

static std::mutex m1;
static std::mutex m2;
static std::latch both_ready{2};

static void worker_a() {
    std::lock_guard<std::mutex> hold_m1(m1);
    both_ready.arrive_and_wait();
    std::lock_guard<std::mutex> hold_m2(m2);  // 等 worker_b 手里的 m2
}

static void worker_b() {
    std::lock_guard<std::mutex> hold_m2(m2);
    both_ready.arrive_and_wait();
    std::lock_guard<std::mutex> hold_m1(m1);  // 等 worker_a 手里的 m1
}

int main() {
    std::thread a(worker_a);
    std::thread b(worker_b);
    std::cout << "main: 两个线程已启动，等它们结束\n";
    a.join();
    b.join();
    std::cout << "main: 不会到这里\n";
    return 0;
}
