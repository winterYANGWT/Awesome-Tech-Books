#include <chrono>
#include <iostream>

#include "task.hpp"
#include "timer_awaiter.hpp"

using namespace std::chrono_literals;

task<int> ticker() {
    for (int i = 1; i <= 3; ++i) {
        co_await timer_awaiter{100ms};
        std::cout << "tick " << i << "\n";
    }
    co_return 42;
}

int main() {
    task<int> t = ticker();
    t.resume();  // 第一次进入，执行到第一个 co_await
    while (!t.done()) {
        if (!run_one_timer()) break;
    }
    std::cout << "result=" << t.result() << "\n";
}
