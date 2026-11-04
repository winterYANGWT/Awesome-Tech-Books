// mode: run
// expect: main: 写入 data=42
// expect: consumer: 被唤醒后读到 42
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

static std::mutex m;
static std::condition_variable cv;
static int data = 0;
static bool ready = false;

static void consumer() {
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, [] { return ready; });  // 谓词为假就继续等，不靠睡多久
    std::cout << "consumer: 被唤醒后读到 " << data << "\n";
}

int main() {
    std::thread t(consumer);

    {
        std::lock_guard<std::mutex> lk(m);
        data = 42;
        ready = true;
        std::cout << "main: 写入 data=42\n";
    }
    cv.notify_one();
    t.join();
    return 0;
}
