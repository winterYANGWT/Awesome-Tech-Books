// 第 30.2 节：std::thread 上的异常没有通道回到调用方，只能 terminate
// mode: crash
// sanitize: none
// flags: -pthread
// expect: terminate
#include <iostream>
#include <stdexcept>
#include <thread>

void process() {
    std::cout << "工作线程开始\n";
    throw std::runtime_error("order failed");
}

int main() {
    std::thread t(process);
    t.join();                      // 异常已经在线程内部触发 std::terminate
    std::cout << "这一行不会执行\n";
}
