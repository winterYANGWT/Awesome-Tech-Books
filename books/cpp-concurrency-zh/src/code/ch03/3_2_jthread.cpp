// mode: run
// 离开作用域时不写 join：jthread 先 request_stop，再 join。
#include <chrono>
#include <cstdio>
#include <stop_token>
#include <thread>

namespace {

void ticker(std::stop_token token) {
    while (!token.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::printf("工作线程：看到 stop_requested，退出\n");
}

}  // namespace

int main() {
    {
        std::jthread worker(ticker);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::printf("主线程：睡 50 ms 后离开作用域\n");
    }
    std::printf("主线程：jthread 析构完成\n");
    return 0;
}
