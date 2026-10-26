// 第 31.2 节：co_await 依次调用 await_ready、await_suspend、await_resume
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

struct Probe {
    const char* name;
    bool ready;

    bool await_ready() const noexcept {
        std::cout << "  await_ready(" << name << ") -> "
                  << (ready ? "true" : "false") << "\n";
        return ready;
    }
    void await_suspend(std::coroutine_handle<>) const noexcept {
        std::cout << "  await_suspend(" << name << ")\n";
    }
    int await_resume() const noexcept {
        std::cout << "  await_resume(" << name << ")\n";
        return 42;
    }
};

Task probes() {
    std::cout << "body: before the first co_await\n";
    int first = co_await Probe{"not-ready", false};
    std::cout << "body: first = " << first << "\n";

    std::cout << "body: before the second co_await\n";
    int second = co_await Probe{"ready", true};
    std::cout << "body: second = " << second << "\n";
    co_return;
}

int main() {
    Task task = probes();
    std::cout << "main: created, done=" << task.done() << "\n";

    task.resume();
    std::cout << "main: back from resume 1, done=" << task.done() << "\n";

    task.resume();
    std::cout << "main: back from resume 2, done=" << task.done() << "\n";
}
