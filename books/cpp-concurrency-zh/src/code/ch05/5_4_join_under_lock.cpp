// mode: hang
// expect: 仍未结束
#include <iostream>
#include <mutex>
#include <thread>

std::mutex m;

void worker() {
    std::cout << "工作线程：请求 m\n" << std::flush;
    std::lock_guard<std::mutex> lock(m);
    std::cout << "工作线程：拿到 m，结束\n";
}

int main() {
    std::lock_guard<std::mutex> lock(m);
    std::cout << "主线程：持有 m，准备 join\n" << std::flush;
    std::thread t(worker);
    t.join();  // 主线程持锁等待：工作线程拿不到 m，就结束不了
    std::cout << "结束\n";
}
