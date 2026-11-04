// mode: run
// expect: 弹出 20000 个，校验和 200010000
// 检查和取数在同一把锁内完成：pop() 返回 optional，空栈用 nullopt 表示。
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <optional>
#include <stack>
#include <thread>
#include <vector>

namespace {

template <class T>
class LockedStack {
public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(m_);
        stack_.push(std::move(value));
    }

    // ANCHOR: pop
    std::optional<T> pop() {
        std::lock_guard<std::mutex> lock(m_);
        if (stack_.empty()) {
            return std::nullopt;
        }
        T value = std::move(stack_.top());
        stack_.pop();
        return value;
    }
    // ANCHOR_END: pop

private:
    std::mutex m_;
    std::stack<T> stack_;
};

}  // namespace

int main() {
    constexpr int kItems = 20000;
    LockedStack<int> stack;
    for (int i = 1; i <= kItems; ++i) {
        stack.push(i);
    }

    std::atomic<int> popped{0};
    std::atomic<std::uint64_t> checksum{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 2; ++t) {
        threads.emplace_back([&] {
            std::uint64_t sum = 0;
            int n = 0;
            while (auto value = stack.pop()) {
                sum += static_cast<std::uint64_t>(*value);
                ++n;
            }
            checksum.fetch_add(sum, std::memory_order_relaxed);
            popped.fetch_add(n, std::memory_order_relaxed);
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }

    std::printf("弹出 %d 个，校验和 %llu\n", popped.load(),
                static_cast<unsigned long long>(checksum.load()));
    return 0;
}
