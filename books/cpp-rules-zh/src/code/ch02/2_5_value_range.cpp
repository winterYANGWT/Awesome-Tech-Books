// 第 2.5 节：先定业务上限，再选装得下的类型
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>

// 第 93 个 Fibonacci 数还在 uint64_t 里，第 94 个就超了。
std::optional<std::uint64_t> fib(unsigned position) {
    constexpr unsigned max_position = 93;
    if (position < 1 || position > max_position) return std::nullopt;

    std::uint64_t a = 0, b = 1;
    for (unsigned i = 1; i < position; ++i) {
        // 即使选了 uint64_t，也要在加之前检查，而不是加完再看。
        if (a > std::numeric_limits<std::uint64_t>::max() - b) return std::nullopt;
        const std::uint64_t next = a + b;
        a = b;
        b = next;
    }
    return b;
}

int main() {
    if (auto v = fib(10)) std::cout << "fib(10) = " << *v << '\n';
    if (auto v = fib(93)) std::cout << "fib(93) = " << *v << '\n';
    if (auto v = fib(94)) {
        std::cout << "fib(94) = " << *v << '\n';
    } else {
        std::cout << "fib(94) 超出 uint64_t 上限\n";
    }

    // 上限不是猜的：F(93) 仍小于 uint64_t 的最大值。
    static_assert(std::numeric_limits<std::uint64_t>::max() >= 12200160415121876738ull);
}
