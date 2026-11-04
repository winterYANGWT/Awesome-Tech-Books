// mode: run
// expect: 消费者进入等待\n主线程 request_stop\nstop_callback 执行：notify_all\n消费者被唤醒，stop_requested=1\n消费者退出
// 普通 condition_variable 没有 stop_token 重载；用 stop_callback 注册一个
// 只做 notify_all 的回调，停止请求会在调用 request_stop 的线程里执行它。
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <queue>
#include <stop_token>
#include <thread>

using namespace std::chrono_literals;

namespace {

std::mutex m;
std::condition_variable cv;
std::queue<int> q;

}  // namespace

int main() {
    std::jthread consumer([](std::stop_token token) {
        std::stop_callback wake(token, [] {
            std::printf("stop_callback 执行：notify_all\n");
            std::fflush(stdout);
            cv.notify_all();
        });

        std::unique_lock<std::mutex> lock(m);
        std::printf("消费者进入等待\n");
        std::fflush(stdout);
        cv.wait(lock, [&] { return !q.empty() || token.stop_requested(); });
        std::printf("消费者被唤醒，stop_requested=%d\n", token.stop_requested() ? 1 : 0);
        std::printf("消费者退出\n");
    });

    std::this_thread::sleep_for(50ms);
    std::printf("主线程 request_stop\n");
    std::fflush(stdout);
    consumer.request_stop();
    consumer.join();
    return 0;
}
