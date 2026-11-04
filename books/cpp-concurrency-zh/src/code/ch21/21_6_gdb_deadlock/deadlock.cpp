// 两个资金账户各有一把锁。transfer_a 先拿 account 再拿 log，transfer_b
// 相反；两个线程都拿到第一把后，再去拿对方持有的那一把。
// 用 latch 把“都拿到第一把”摆到同一个点上，死锁每次都复现。
#include <chrono>
#include <cstdio>
#include <latch>
#include <mutex>
#include <pthread.h>
#include <thread>

namespace {

std::mutex account_mutex;
std::mutex log_mutex;
std::latch both_ready{2};

void rename_self(const char* name) {
    (void)pthread_setname_np(pthread_self(), name);
}

void transfer_a() {
    rename_self("transfer-a");
    std::lock_guard<std::mutex> account(account_mutex);
    std::printf("transfer-a 持有 account_mutex\n");
    std::fflush(stdout);
    both_ready.count_down();
    both_ready.wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::lock_guard<std::mutex> log(log_mutex);  // 等 transfer-b 放 log_mutex
    std::printf("transfer-a 拿到 log_mutex\n");
}

void transfer_b() {
    rename_self("transfer-b");
    std::lock_guard<std::mutex> log(log_mutex);
    std::printf("transfer-b 持有 log_mutex\n");
    std::fflush(stdout);
    both_ready.count_down();
    both_ready.wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::lock_guard<std::mutex> account(account_mutex);  // 等 transfer-a 放 account_mutex
    std::printf("transfer-b 拿到 account_mutex\n");
}

}  // namespace

int main() {
    std::thread a(transfer_a);
    std::thread b(transfer_b);
    std::printf("主线程等待两个工作线程 join\n");
    std::fflush(stdout);
    a.join();
    b.join();
    std::printf("不会走到这里\n");
    return 0;
}
