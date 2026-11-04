// mode: run
// expect: 成功数: 3
// expect: 异常数: 1
// expect: 异常消息: 任务 2 失败
// 4 个任务里第 3 个抛异常；主线程逐个 get，把每个异常都收下来。
#include <cstdio>
#include <exception>
#include <future>
#include <stdexcept>
#include <vector>

int main() {
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 4; ++i) {
        futures.push_back(std::async(std::launch::async, [i] {
            if (i == 2) {
                throw std::runtime_error("任务 2 失败");
            }
            return i * 10;
        }));
    }

    int succeeded = 0;
    int failed = 0;
    std::vector<std::exception_ptr> errors;
    for (std::future<int>& f : futures) {
        try {
            (void)f.get();
            ++succeeded;
        } catch (...) {
            ++failed;
            errors.push_back(std::current_exception());
        }
    }

    std::printf("成功数: %d\n", succeeded);
    std::printf("异常数: %d\n", failed);
    for (const std::exception_ptr& error : errors) {
        try {
            std::rethrow_exception(error);
        } catch (const std::exception& e) {
            std::printf("异常消息: %s\n", e.what());
        }
    }
}
