// 第 30.2 节：析构一个仍 joinable 的 std::thread 会直接 terminate
// mode: crash
// sanitize: none
// flags: -pthread
// expect: terminate
#include <iostream>
#include <thread>

bool runTask(bool conditionsOk) {
    std::thread t([] { std::cout << "后台线程运行\n"; });
    if (!conditionsOk) return false;   // 这里直接返回，t 仍 joinable
    t.join();
    return true;
}

int main() {
    runTask(false);
    std::cout << "这一行不会执行\n";
}
