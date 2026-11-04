// mode: run
// expect: worker: 处理第 3 个任务
// expect: stopped after 3
// expect: main: 线程已汇入
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stop_token>
#include <thread>

static std::mutex m;
static std::condition_variable_any cv;
static std::condition_variable done_cv;
static bool ready = false;
static int processed = 0;

static void worker(std::stop_token st) {
    std::unique_lock<std::mutex> lk(m);
    for (;;) {
        // 带 stop_token 的等待：停止请求会把它唤醒，返回谓词的值。
        bool has_work = cv.wait(lk, st, [&] { return ready || st.stop_requested(); });
        if (st.stop_requested()) {
            break;
        }
        if (!has_work) {
            break;
        }
        ready = false;
        ++processed;
        std::cout << "worker: 处理第 " << processed << " 个任务\n";
        done_cv.notify_one();
    }
    std::cout << "stopped after " << processed << "\n";
}

int main() {
    std::jthread t(worker);  // worker 收 stop_token，jthread 自动把它作为第一个实参传入

    for (int i = 1; i <= 3; ++i) {
        {
            std::lock_guard<std::mutex> lk(m);
            ready = true;
        }
        cv.notify_one();

        std::unique_lock<std::mutex> lk(m);
        done_cv.wait(lk, [&] { return processed == i; });
    }

    t.request_stop();
    cv.notify_all();  // 带 token 的 wait 自己会被通知；这里再叫一次，意图更明确
    t.join();
    std::cout << "main: 线程已汇入\n";
    return 0;
}
