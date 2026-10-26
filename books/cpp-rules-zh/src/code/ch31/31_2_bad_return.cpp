// 第 31.2 节：promise_type 只提供 return_void，co_return 一个值就编译不过
// mode: fail
// expect: return_value
#include <coroutine>
#include <exception>

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

    explicit Task(std::coroutine_handle<promise_type> h) : h_(h) {}
    ~Task() {
        if (h_) h_.destroy();
    }

private:
    std::coroutine_handle<promise_type> h_;
};

Task wrong() {
    co_return 1;
}

int main() {
    Task task = wrong();
}
