// mode: run
// 最小 task<int>：协程不创建线程，resume 两次才走完“开始/恢复/返回”。
#include <coroutine>
#include <exception>
#include <iostream>

class task {
public:
    struct promise_type {
        int value = 0;
        std::exception_ptr error;

        task get_return_object() {
            return task(std::coroutine_handle<promise_type>::from_promise(*this));
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_value(int v) { value = v; }
        void unhandled_exception() { error = std::current_exception(); }
    };

    explicit task(std::coroutine_handle<promise_type> h) : handle_(h) {}
    ~task() {
        if (handle_) {
            handle_.destroy();
        }
    }
    task(const task&) = delete;
    task& operator=(const task&) = delete;
    task(task&& other) noexcept : handle_(other.handle_) { other.handle_ = {}; }

    void resume() { handle_.resume(); }
    bool done() const { return handle_.done(); }
    int result() const { return handle_.promise().value; }

private:
    std::coroutine_handle<promise_type> handle_;
};

task compute() {
    std::cout << "开始\n";
    co_await std::suspend_always{};
    std::cout << "恢复\n";
    co_return 42;
}

int main() {
    auto t = compute();
    std::cout << "创建后 done=" << t.done() << '\n';
    t.resume();
    std::cout << "第一次 resume 后 done=" << t.done() << '\n';
    t.resume();
    std::cout << "第二次 resume 后 done=" << t.done() << '\n';
    std::cout << "返回 " << t.result() << '\n';
}
