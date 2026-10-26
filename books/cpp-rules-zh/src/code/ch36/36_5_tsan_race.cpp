// 第 36.5 节：TSan 抓住两个线程对同一变量的无同步写
// mode: crash
// sanitize: thread
// expect: data race
#include <iostream>
#include <thread>

int accepted = 0;

void record_accept() {
    for (int i = 0; i < 1000; ++i) {
        ++accepted;  // 非原子，两个线程同时写
    }
}

int main() {
    std::thread a(record_accept);
    std::thread b(record_accept);
    a.join();
    b.join();
    std::cout << "accepted=" << accepted << '\n';
}
