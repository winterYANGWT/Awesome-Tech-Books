// 第 31.3 节：C++23 的 std::generator 是惰性序列，协程体在调用 resume 的线程上运行
// std: c++23
#include <generator>
#include <iostream>
#include <thread>

std::generator<int> squares(int n, std::thread::id expected) {
    for (int i = 0; i < n; ++i) {
        bool on_expected = std::this_thread::get_id() == expected;
        std::cout << "  produce " << i << ", on expected thread? "
                  << (on_expected ? "true" : "false") << "\n";
        co_yield i * i;
    }
}

int main() {
    std::thread::id main_id = std::this_thread::get_id();
    std::thread::id worker_id{};

    std::thread worker([&] {
        worker_id = std::this_thread::get_id();
        int count = 0;
        for (int v : squares(5, worker_id)) {
            std::cout << "worker got " << v << "\n";
            if (++count == 2) break;
        }
    });
    worker.join();

    std::cout << "main and worker are different threads? "
              << (main_id != worker_id ? "true" : "false") << "\n";
}
