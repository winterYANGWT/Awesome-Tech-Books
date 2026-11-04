// mode: crash
// expect: 创建线程，马上离开函数
// expect: terminate called without an active exception
#include <iostream>
#include <thread>

static void start_background() {
    std::thread t([] {
        // 线程做完就退出，但 t 仍然是 joinable 的。
    });
    std::cout << "创建线程，马上离开函数\n";
    // t 在这里析构：joinable() 为真，std::thread 的析构调用 std::terminate。
}

int main() {
    start_background();
    std::cout << "这一行不会执行\n";
    return 0;
}
