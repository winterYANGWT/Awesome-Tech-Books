// mode: run
// 没有标准 when_all 时，用两个 async 加两次 get 手工等待和合并。
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;

int main() {
    auto a = std::async(std::launch::async, [] {
        std::this_thread::sleep_for(30ms);
        return 20;
    });
    auto b = std::async(std::launch::async, [] {
        std::this_thread::sleep_for(10ms);
        return 22;
    });

    // 先阻塞在 a 上，再阻塞在 b 上；两个都返回后才能合并。
    const int va = a.get();
    const int vb = b.get();
    std::cout << "a=" << va << " b=" << vb << " sum=" << va + vb << '\n';
}
