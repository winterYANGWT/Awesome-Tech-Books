// mode: crash
// expect: terminate called without an active exception
// main 创建线程后既不 join 也不 detach，直接返回；作用域结束时 std::thread
// 仍 joinable，析构调用 terminate。把 return 换成 throw，也会走同一个析构。
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    std::thread t([] {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    });
    std::cout << "线程对象 joinable=" << t.joinable()
              << "，main 不 join、不 detach，直接返回" << std::endl;
    return 0;
}
