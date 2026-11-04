// mode: hang
// expect: 仍未结束
#include <functional>
#include <iostream>
#include <latch>
#include <mutex>
#include <thread>

std::mutex m1, m2;
std::latch both_ready(2);

// 两个线程用相反的顺序拿两把锁：A 先 m1，B 先 m2。
void transfer(std::mutex& first, std::mutex& second, const char* name) {
    std::unique_lock<std::mutex> lock1(first);
    std::cout << name << " 拿到第一把锁\n" << std::flush;

    // 为了稳定复现，把两个线程都推到“各持一把”的位置再继续。
    both_ready.arrive_and_wait();

    std::cout << name << " 请求第二把锁\n" << std::flush;
    std::unique_lock<std::mutex> lock2(second);
    std::cout << name << " 两把都拿到\n";
}

int main() {
    std::thread a(transfer, std::ref(m1), std::ref(m2), "A");
    std::thread b(transfer, std::ref(m2), std::ref(m1), "B");
    a.join();
    b.join();
}
