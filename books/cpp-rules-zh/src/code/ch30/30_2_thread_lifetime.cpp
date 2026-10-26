// 第 30.2 节：std::thread 要在每条路径上变得不可 join，C++20 可用 jthread
// flags: -pthread
#include <atomic>
#include <iostream>
#include <thread>
#include <utility>

// 最小的 ThreadRAII：析构时按策略 join 或 detach，绝不让 joinable 的 thread 被销毁。
class ThreadRAII {
public:
    enum class Action { join, detach };

    ThreadRAII(std::thread&& t, Action a) : action_(a), t_(std::move(t)) {}
    ~ThreadRAII() {
        if (t_.joinable()) {
            if (action_ == Action::join) t_.join();
            else t_.detach();
        }
    }
    ThreadRAII(const ThreadRAII&) = delete;
    ThreadRAII& operator=(const ThreadRAII&) = delete;

    std::thread& get() { return t_; }

private:
    Action action_;
    std::thread t_;              // 最后声明：构造时其他成员已经就绪
};

bool importBatch(bool conditionsOk, std::atomic<int>& processed) {
    ThreadRAII worker(std::thread([&processed] {
        for (int i = 0; i < 1000; ++i) processed.fetch_add(1, std::memory_order_relaxed);
    }), ThreadRAII::Action::join);

    if (!conditionsOk) return false;   // 提前返回，worker 的析构仍会 join
    return true;
}

int main() {
    std::cout << std::boolalpha;
    std::atomic<int> processed{0};
    const bool ok = importBatch(false, processed);
    std::cout << "早退路径返回 " << ok << "，已处理 " << processed.load() << '\n';

    {
        std::jthread autoJoin([&processed] {
            processed.fetch_add(1, std::memory_order_relaxed);
        });
    }                                   // jthread 析构：request_stop + join
    std::cout << "jthread 块结束后已 join，已处理 " << processed.load() << '\n';

    std::atomic<long> ticks{0};
    {
        std::jthread spinner([&ticks](std::stop_token st) {
            while (!st.stop_requested()) {
                ticks.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::yield();
            }
        });
        // 等 spinner 至少跑一轮，再离开作用域请求停止。
        while (ticks.load(std::memory_order_relaxed) == 0) std::this_thread::yield();
    }                                   // 析构先请求停止，再 join
    std::cout << "stop_token 生效，ticks 大于 0: " << (ticks.load() > 0) << '\n';
}
