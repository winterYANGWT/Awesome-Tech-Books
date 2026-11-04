// mode: fail
// expect: await_ready|await_suspend|no member named 'await_ready'
// 注意：std::future 没有 awaiter，不能直接 co_await。
#include <coroutine>
#include <exception>
#include <future>

struct task {
    struct promise_type {
        std::exception_ptr error;

        task get_return_object() {
            return task(std::coroutine_handle<promise_type>::from_promise(*this));
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
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

    std::coroutine_handle<promise_type> handle_;
};

task run() {
    auto f = std::async(std::launch::async, [] { return 1; });
    int x = co_await std::move(f); // std::future 没有 await_ready/await_suspend
    (void)x;
}
