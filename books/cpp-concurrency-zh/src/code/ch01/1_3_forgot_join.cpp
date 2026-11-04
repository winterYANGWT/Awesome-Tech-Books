// mode: crash
// expect: terminate called without an active exception
#include <iostream>
#include <thread>

void worker() {
    std::cout << "工作线程在跑\n";
}

int main() {
    {
        std::thread t(worker);
        // 故意不 join：t 离开作用域时仍 joinable，标准要求调用 terminate。
    }
    std::cout << "这一行不会打印\n";
}
