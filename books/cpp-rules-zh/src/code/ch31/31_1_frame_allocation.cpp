// 第 31.1 节：协程帧通常会分配，分配和释放可以经 promise_type::operator new 观察
#include <coroutine>
#include <cstddef>
#include <iostream>
#include <utility>

struct Counted {
    struct promise_type {
        static inline int allocations = 0;
        static inline int frees = 0;

        static void* operator new(std::size_t size) {
            ++allocations;
            std::cout << "  promise_type::operator new\n";
            return ::operator new(size);
        }
        static void operator delete(void* p, std::size_t) noexcept {
            ++frees;
            std::cout << "  promise_type::operator delete\n";
            ::operator delete(p);
        }

        Counted get_return_object() {
            return Counted{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { std::terminate(); }
    };

    using handle = std::coroutine_handle<promise_type>;

    explicit Counted(handle h) : h_(h) {}
    Counted(Counted&& other) noexcept : h_(std::exchange(other.h_, {})) {}
    Counted(const Counted&) = delete;
    Counted& operator=(const Counted&) = delete;
    ~Counted() {
        if (h_) h_.destroy();
    }

    void resume() { h_.resume(); }

private:
    handle h_;
};

Counted job(int id) {
    std::cout << "job " << id << " starts\n";
    co_await std::suspend_always{};
    std::cout << "job " << id << " resumes\n";
}

int main() {
    std::cout << "entering scope\n";
    {
        Counted a = job(1);
        Counted b = job(2);
        std::cout << "created two coroutines: allocations="
                  << Counted::promise_type::allocations
                  << ", frees=" << Counted::promise_type::frees << "\n";

        a.resume();
        b.resume();
        std::cout << "both resumed: frees=" << Counted::promise_type::frees << "\n";
    }
    std::cout << "after scope: allocations=" << Counted::promise_type::allocations
              << ", frees=" << Counted::promise_type::frees << "\n";
}
