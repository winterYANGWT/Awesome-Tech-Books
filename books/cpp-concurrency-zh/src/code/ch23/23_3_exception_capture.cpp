// mode: run
// flags: -ltbb
// tsan: no（TBB 运行时未插桩，会误报）

#include <algorithm>
#include <cstdio>
#include <exception>
#include <execution>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <vector>

int main() {
    std::vector<int> v(100);
    std::iota(v.begin(), v.end(), 0);

    std::mutex slot_mutex;
    std::exception_ptr first_error;

    std::for_each(std::execution::par, v.begin(), v.end(), [&](int i) {
        try {
            if (i == 7) {
                throw std::runtime_error("boom");
            }
        } catch (...) {
            // 只保留第一个异常；exception_ptr 本身由互斥量保护。
            std::lock_guard<std::mutex> lock(slot_mutex);
            if (!first_error) {
                first_error = std::current_exception();
            }
        }
    });

    // 算法已经返回，现在异常可以沿普通调用栈传播。
    try {
        if (first_error) {
            std::rethrow_exception(first_error);
        }
        std::puts("no error");
    } catch (const std::exception& e) {
        std::printf("caught: %s\n", e.what());
    }
}
