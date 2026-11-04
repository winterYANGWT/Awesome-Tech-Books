// mode: hang
// expect: 仍未结束
#include <functional>
#include <iostream>
#include <latch>
#include <mutex>
#include <thread>

// 同一段 ABBA 代码，用 latch 固定交错后每一轮都会挂住。
void transfer(std::mutex& first, std::mutex& second, std::latch& both_ready) {
    std::unique_lock<std::mutex> lock1(first);
    both_ready.arrive_and_wait();
    std::unique_lock<std::mutex> lock2(second);
}

int main() {
    for (int round = 1; round <= 1000; ++round) {
        std::mutex m1, m2;
        std::latch both_ready(2);
        std::cout << "第 " << round << " 次\n" << std::flush;

        std::thread a(transfer, std::ref(m1), std::ref(m2), std::ref(both_ready));
        std::thread b(transfer, std::ref(m2), std::ref(m1), std::ref(both_ready));
        a.join();
        b.join();
    }
    std::cout << "1000 次都完成了\n";
}
