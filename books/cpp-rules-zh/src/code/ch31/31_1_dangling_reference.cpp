// 第 31.1 节：跨越挂起点保存指向调用方临时数据的指针，恢复时就是悬垂指针
// mode: crash
// expect: heap-use-after-free
#include <coroutine>
#include <iostream>
#include <string>
#include <utility>

struct Task {
    struct promise_type {
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        // 这里让函数体立刻执行，先保存指针，再在 co_await 处挂起
        std::suspend_never initial_suspend() noexcept { return {}; }
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

    void resume() { h_.resume(); }

private:
    handle h_;
};

Task print_first_word(const std::string& text) {
    const char* data = text.data();  // 指向调用方临时 string 的堆缓冲区
    co_await std::suspend_always{};
    char first = data[0];  // 临时 string 已经销毁，这里读的是已释放的缓冲区
    std::cout << "first character: " << first << "\n";
}

int main() {
    std::cout << "creating a task that borrows a temporary\n";
    Task task = print_first_word(std::string("customer-00042-with-a-long-id"));
    std::cout << "the temporary is gone, now resuming\n";
    task.resume();
}
