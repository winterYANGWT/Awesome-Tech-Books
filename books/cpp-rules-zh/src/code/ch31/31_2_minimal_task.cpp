// 第 31.2 节：用 promise_type 和对称转移写一个能嵌套、能传播异常的 Task
#include <coroutine>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

template <class T>
class Task {
public:
    struct promise_type {
        std::optional<T> value;
        std::exception_ptr error;
        std::coroutine_handle<> continuation{};

        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }

        struct FinalAwaiter {
            bool await_ready() const noexcept { return false; }
            std::coroutine_handle<> await_suspend(
                std::coroutine_handle<promise_type> h) const noexcept {
                auto next = h.promise().continuation;
                return next ? next : std::noop_coroutine();
            }
            void await_resume() const noexcept {}
        };
        FinalAwaiter final_suspend() noexcept { return {}; }

        void return_value(T v) { value.emplace(std::move(v)); }
        void unhandled_exception() noexcept { error = std::current_exception(); }
    };

    using handle = std::coroutine_handle<promise_type>;

    explicit Task(handle h) : h_(h) {}
    Task(Task&& other) noexcept : h_(std::exchange(other.h_, {})) {}
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    ~Task() {
        if (h_) h_.destroy();
    }

    bool await_ready() const noexcept { return !h_ || h_.done(); }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
        h_.promise().continuation = caller;
        return h_;
    }
    T await_resume() {
        if (h_.promise().error) std::rethrow_exception(h_.promise().error);
        return std::move(*h_.promise().value);
    }

    void resume() { h_.resume(); }
    T result() { return await_resume(); }

private:
    handle h_;
};

Task<int> parse_id(std::string raw) {
    if (raw.empty()) throw std::runtime_error("empty order id");
    co_return std::stoi(raw);
}

Task<int> load_order(std::string raw) {
    std::cout << "load_order: before co_await\n";
    int id = co_await parse_id(raw);
    std::cout << "load_order: after co_await, id=" << id << "\n";
    co_return id * 10;
}

int main() {
    std::cout << "main: create load_order\n";
    Task<int> order = load_order("42");
    std::cout << "main: resume it\n";
    order.resume();
    std::cout << "main: result=" << order.result() << "\n";

    std::cout << "main: create a failing load_order\n";
    Task<int> bad = load_order("");
    bad.resume();
    try {
        int value = bad.result();
        std::cout << "main: result=" << value << "\n";
    } catch (const std::exception& e) {
        std::cout << "main: caught " << e.what() << "\n";
    }
}
