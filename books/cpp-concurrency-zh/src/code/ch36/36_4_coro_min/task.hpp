// 最小 task<T>：不依赖任何协程库，只实现 co_return 和 co_await 需要的部分。
#pragma once

#include <coroutine>
#include <exception>
#include <utility>

template <class T>
class task {
public:
    // ANCHOR: promise
    struct promise_type {
        T value{};
        std::exception_ptr error;

        task get_return_object();

        std::suspend_always initial_suspend() noexcept { return {}; }
        // 结束时挂起，让调用方还有机会读 value；由 task 的析构函数销毁帧。
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_value(T v) { value = std::move(v); }
        void unhandled_exception() { error = std::current_exception(); }
    };

    // ANCHOR_END: promise
    using handle_type = std::coroutine_handle<promise_type>;

    task() = default;
    explicit task(handle_type h) : handle_(h) {}
    task(task&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}
    task& operator=(task&& other) noexcept {
        if (this != &other) {
            if (handle_) handle_.destroy();
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }
    task(const task&) = delete;
    task& operator=(const task&) = delete;
    ~task() {
        if (handle_) handle_.destroy();
    }

    bool done() const { return handle_ && handle_.done(); }
    void resume() { handle_.resume(); }
    T result() {
        if (handle_.promise().error) std::rethrow_exception(handle_.promise().error);
        return std::move(handle_.promise().value);
    }

private:
    handle_type handle_{};
};

template <class T>
task<T> task<T>::promise_type::get_return_object() {
    return task<T>{handle_type::from_promise(*this)};
}
