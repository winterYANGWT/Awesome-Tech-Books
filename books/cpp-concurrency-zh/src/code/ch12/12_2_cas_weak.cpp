// mode: run
// output: varies
// expect: 伪失败
#include <atomic>
#include <cstdint>
#include <iostream>

int main() {
    constexpr int kAttempts = 1'000'000;
    std::atomic<std::uint64_t> value{0};
    std::uint64_t failures = 0;
    std::uint64_t spurious = 0;

    for (int i = 0; i < kAttempts; ++i) {
        std::uint64_t expected = value.load();
        const std::uint64_t desired = expected + 1;
        if (!value.compare_exchange_weak(expected, desired)) {
            ++failures;
            // 值没变却返回 false，才是“伪失败”
            if (expected == value.load()) {
                ++spurious;
            }
        }
    }

    std::cout << "尝试 " << kAttempts << " 次\n";
    std::cout << "失败 " << failures << " 次，其中伪失败 " << spurious << " 次\n";
    std::cout << "最终值 = " << value.load() << '\n';
}
