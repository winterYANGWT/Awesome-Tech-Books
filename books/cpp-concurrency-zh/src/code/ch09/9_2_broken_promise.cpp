// mode: crash
// expect: broken_promise: 是
// expect: Broken promise
// promise 在设值前离开作用域；future 以 broken_promise 就绪。
#include <cstdio>
#include <future>
#include <utility>

int main() {
    std::future<int> result;
    {
        std::promise<int> promise;
        result = promise.get_future();
        // 没有 set_value()，promise 在这里析构。
    }
    std::printf("promise 在设值前离开作用域\n");
    std::fflush(stdout);

    try {
        const int value = result.get();
        std::printf("不应该走到这里: %d\n", value);
    } catch (const std::future_error& e) {
        std::printf("future_error: %s\n", e.what());
        std::printf("broken_promise: %s\n",
                    e.code() == std::future_errc::broken_promise ? "是" : "否");
        std::fflush(stdout);
        throw;  // 没有外层 catch，程序 terminate。
    }
}
