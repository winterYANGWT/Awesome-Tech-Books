// mode: run
// std: c++23
// expect: function_wrapper: 42
// expect: move_only_function: 7
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>

// C++11/17 的旧写法：虚函数 + unique_ptr，目标只要求可移动。
class function_wrapper {
    struct impl_base {
        virtual void call() = 0;
        virtual ~impl_base() = default;
    };

    template <class F>
    struct impl_type : impl_base {
        F f;
        explicit impl_type(F &&f_) : f(std::move(f_)) {}
        void call() override { f(); }
    };

    std::unique_ptr<impl_base> impl;

public:
    template <class F>
    explicit function_wrapper(F &&f)
        : impl(new impl_type<std::decay_t<F>>(std::forward<F>(f))) {}

    void operator()() { impl->call(); }

    function_wrapper(function_wrapper &&) = default;
    function_wrapper &operator=(function_wrapper &&) = default;
    function_wrapper(const function_wrapper &) = delete;
    function_wrapper &operator=(const function_wrapper &) = delete;
};

int main() {
    std::packaged_task<int()> task([]{ return 42; });
    auto f1 = task.get_future();
    function_wrapper w(std::move(task));
    w();
    std::cout << "function_wrapper: " << f1.get() << "\n";

    std::packaged_task<int()> task2([]{ return 7; });
    auto f2 = task2.get_future();
    std::move_only_function<void()> mf(std::move(task2));
    mf();
    std::cout << "move_only_function: " << f2.get() << "\n";
    return 0;
}
