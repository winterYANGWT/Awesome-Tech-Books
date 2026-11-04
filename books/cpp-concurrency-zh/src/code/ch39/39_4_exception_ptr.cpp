// mode: run
// packaged_task 把异常存进 shared state，future.get() 在调用线程重抛。
#include <exception>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

const char* type_name(const std::exception_ptr& ep) {
    try {
        std::rethrow_exception(ep);
    } catch (const std::runtime_error&) {
        return "runtime_error";
    } catch (const std::exception&) {
        return "other std::exception";
    } catch (...) {
        return "unknown";
    }
}

int main() {
    std::packaged_task<int()> task([]() -> int {
        throw std::runtime_error("disk full");
    });
    auto f = task.get_future();
    std::thread worker(std::move(task));
    worker.join();

    try {
        (void)f.get();
    } catch (const std::runtime_error& e) {
        std::cout << "packaged_task: " << e.what() << '\n';
    }

    auto ep = std::make_exception_ptr(std::runtime_error("bad config"));
    try {
        std::rethrow_exception(ep);
    } catch (const std::runtime_error& e) {
        std::cout << "make_exception_ptr: " << e.what() << '\n';
    }
    std::cout << "type: " << type_name(ep) << '\n';
}
