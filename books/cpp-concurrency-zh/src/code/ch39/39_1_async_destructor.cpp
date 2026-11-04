// mode: run
// output: varies
// tsan: no（计时示例）
// expect: 析构阻塞: yes
// future 析构时等任务跑完，而不是取消任务；计时看阻塞的方向。
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;

int main() {
    const auto start = std::chrono::steady_clock::now();
    {
        auto f = std::async(std::launch::async, [] {
            std::this_thread::sleep_for(500ms);
            return 42;
        });
        std::cout << "任务已提交，future 还在作用域里\n";
    } // 这里调用 future 的析构函数
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    std::cout << "future 析构耗时约 " << elapsed << " ms\n";
    std::cout << "析构阻塞: " << (elapsed >= 450 ? "yes" : "no") << '\n';
}
