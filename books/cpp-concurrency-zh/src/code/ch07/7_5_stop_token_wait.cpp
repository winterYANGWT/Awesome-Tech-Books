// mode: run
// expect: 消费者进入等待\n主线程 request_stop\nwait 返回 false，stop_requested=1\n消费者退出
// jthread 里的消费者用 condition_variable_any::wait(lock, token, pred) 等队列；
// 主线程 50 ms 后请求停止，等待返回 false，不需要 1 ms 轮询。
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
std::condition_variable_any cv;
std::queue<int> q;

}  // namespace

int main() {
    std::jthread consumer([](std::stop_token token) {
        std::unique_lock<std::mutex> lock(m);
        std::printf("消费者进入等待\n");
        std::fflush(stdout);
        // 谓词只表达“队列非空”；停止请求会让 wait 返回 false。
        const bool ok = cv.wait(lock, token, [&] { return !q.empty(); });
        std::printf("wait 返回 %s，stop_requested=%d\n", ok ? "true" : "false",
                    token.stop_requested() ? 1 : 0);
        std::printf("消费者退出\n");
    });

    std::this_thread::sleep_for(50ms);
    std::printf("主线程 request_stop\n");
    std::fflush(stdout);
    consumer.request_stop();
    consumer.join();
    return 0;
}
