// mode: run
// 工作线程把异常存进 exception_ptr，主线程在 join 之后重新抛出。
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <thread>

namespace {

class ThreadGuard {
  public:
    explicit ThreadGuard(std::thread& thread) : thread_(thread) {}
    ~ThreadGuard() {
        if (thread_.joinable()) {
            thread_.join();
        }
    }
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;

  private:
    std::thread& thread_;
};

std::exception_ptr worker_error;

void worker() {
    try {
        throw std::runtime_error("工作线程失败");
    } catch (...) {
        worker_error = std::current_exception();
    }
}

}  // namespace

int main() {
    std::thread thread(worker);
    try {
        ThreadGuard guard(thread);
        throw std::runtime_error("主线程在 join 前失败");
    } catch (const std::exception& error) {
        std::printf("主线程捕获：%s\n", error.what());
    }

    if (worker_error != nullptr) {
        try {
            std::rethrow_exception(worker_error);
        } catch (const std::exception& error) {
            std::printf("工作线程异常：%s\n", error.what());
        }
    }
    return 0;
}
