// 第 31.3 节：用 co_yield 写一个惰性序列，函数体在每次取值时才往前走一步
#include <coroutine>
#include <iostream>
#include <utility>

class Countdown {
public:
    struct promise_type {
        int current{};

        Countdown get_return_object() {
            return Countdown{handle::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(int v) noexcept {
            current = v;
            return {};
        }
        void return_void() noexcept {}
        void unhandled_exception() { std::terminate(); }
    };

    using handle = std::coroutine_handle<promise_type>;

    class iterator {
    public:
        iterator() = default;
        explicit iterator(handle h) : h_(h) { advance(); }

        int operator*() const { return h_.promise().current; }
        iterator& operator++() {
            advance();
            return *this;
        }
        bool operator==(const iterator& other) const { return h_ == other.h_; }
        bool operator!=(const iterator& other) const { return !(*this == other); }

    private:
        void advance() {
            if (!h_ || h_.done()) {
                h_ = {};
                return;
            }
            h_.resume();
            if (h_.done()) h_ = {};
        }

        handle h_{};
    };

    explicit Countdown(handle h) : h_(h) {}
    Countdown(Countdown&& other) noexcept : h_(std::exchange(other.h_, {})) {}
    Countdown(const Countdown&) = delete;
    Countdown& operator=(const Countdown&) = delete;
    ~Countdown() {
        if (h_) h_.destroy();
    }

    iterator begin() {
        if (!h_ || h_.done()) return iterator{};
        return iterator{h_};
    }
    iterator end() { return iterator{}; }

private:
    handle h_;
};

struct Cleanup {
    ~Cleanup() { std::cout << "  body: cleanup\n"; }
};

Countdown countdown(int from) {
    Cleanup cleanup;
    std::cout << "  body: start\n";
    for (int i = from; i > 0; --i) {
        std::cout << "  body: about to yield " << i << "\n";
        co_yield i;
    }
    std::cout << "  body: finished\n";
}

int main() {
    std::cout << "create the generator\n";
    Countdown counter = countdown(3);
    std::cout << "created; the body has not run yet\n";

    for (int value : counter) {
        std::cout << "main got " << value << "\n";
        if (value == 2) {
            std::cout << "stop early\n";
            break;
        }
    }
    std::cout << "after the loop\n";
}
