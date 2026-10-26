// 第 31.1 节：co_await、co_return 让函数在挂起点暂停，局部变量留在协程帧里
#include <coroutine>
#include <iostream>
#include <utility>

struct Task {
    struct promise_type {
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { std::terminate(); }
    };

    using handle = std::coroutine_handle<promise_type>;

    explicit Task(handle h) : h_(h) {}
    Task(Task&& other) noexcept : h_(std::exchange(other.h_, {})) {}
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    ~Task() {
        if (h_) h_.destroy();
    }

    bool done() const { return h_.done(); }
    void resume() { h_.resume(); }

private:
    handle h_;
};

struct Pause {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    void await_resume() const noexcept {}
};

Task walkthrough() {
    int step = 0;
    std::cout << "body starts: step=" << step << "\n";
    ++step;
    co_await Pause{};
    std::cout << "body after pause 1: step=" << step << "\n";
    ++step;
    co_await Pause{};
    std::cout << "body after pause 2: step=" << step << "\n";
    co_return;
}

int main() {
    std::cout << "before creating the task\n";
    Task task = walkthrough();
    std::cout << "after creating the task: done=" << task.done() << "\n";

    task.resume();
    std::cout << "after resume 1: done=" << task.done() << "\n";

    task.resume();
    std::cout << "after resume 2: done=" << task.done() << "\n";

    task.resume();
    std::cout << "after resume 3: done=" << task.done() << "\n";
}
