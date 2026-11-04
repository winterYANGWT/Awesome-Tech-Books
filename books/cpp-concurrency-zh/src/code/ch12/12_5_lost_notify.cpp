// mode: hang
// timeout: 2
// expect: 仍未结束
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

std::atomic<int> data_ready{0};
std::atomic<int> other_signal{0};
int data = 0;

void wait_ready() {
    while (data_ready.load() == 0) {
        data_ready.wait(0);  // 等的是 data_ready 上的通知；值已变则立即返回
    }
}

void consumer() {
    wait_ready();
    std::cout << "data = " << data << '\n';
}

void producer() {
    data = 42;
    data_ready.store(1);
    other_signal.notify_one();  // 通知发到了另一个原子变量上
}

int main() {
    std::thread c(consumer);
    // 强制交错：让消费者先阻塞在 wait 上，再看通知发错变量的后果
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::thread p(producer);
    p.join();
    c.join();
}
