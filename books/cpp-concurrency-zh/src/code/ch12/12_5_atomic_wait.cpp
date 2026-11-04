// mode: run
// expect: data = 42
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

std::atomic<bool> ready{false};
int data = 0;

void consumer() {
    while (!ready.load()) {
        // 可能虚假返回，所以醒来后还要重查谓词
        ready.wait(false);
    }
    std::cout << "data = " << data << '\n';
}

int main() {
    std::thread c(consumer);
    // 只是为了让消费者先进入等待；同步仍由 ready 的 store/load 负责
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    data = 42;
    ready.store(true);
    ready.notify_one();

    c.join();
}
