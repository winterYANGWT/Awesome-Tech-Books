// mode: run
// expect: 入栈 4000，出栈 4000，校验和 8002000
// 单锁栈：一把 mutex 包住 std::stack；pop 返回 optional，避免分开调用 top/pop。
#include <atomic>
#include <barrier>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <optional>
#include <stack>
#include <thread>
#include <utility>
#include <vector>

namespace {

template <class T>
class LockedStack {
public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(m_);
        stack_.push(std::move(value));
    }

    std::optional<T> pop() {
        std::lock_guard<std::mutex> lock(m_);
        if (stack_.empty()) {
            return std::nullopt;
        }
        T value = std::move(stack_.top());
        stack_.pop();
        return value;
    }

private:
    std::mutex m_;
    std::stack<T> stack_;
};

}  // namespace

int main() {
    constexpr int kThreads = 4;
    constexpr int kPerThread = 1000;
    LockedStack<int> stack;
    std::barrier sync{kThreads};
    std::atomic<int> popped{0};
    std::atomic<std::uint64_t> checksum{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < kPerThread; ++i) {
                stack.push(t * kPerThread + i + 1);
            }
            sync.arrive_and_wait();  // 四个线程都推完，再开始弹
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

    std::printf("入栈 %d，出栈 %d，校验和 %llu\n", kThreads * kPerThread, popped.load(),
                static_cast<unsigned long long>(checksum.load()));
    return 0;
}
