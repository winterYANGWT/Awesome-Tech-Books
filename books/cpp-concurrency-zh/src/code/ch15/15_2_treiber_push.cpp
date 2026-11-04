// mode: run
// expect: push 次数: 800000
// expect: 遍历节点数: 800000
// expect: 值的和与闭式解一致: 是
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

struct Node {
    int value;
    Node* next;
};

std::atomic<Node*> head{nullptr};

void push(int value) {
    Node* node = new Node{value, head.load(std::memory_order_relaxed)};
    // 失败时 compare_exchange_weak 把最新 head 写回 node->next，
    // 所以下一轮直接用新的 node->next 重试，不必再 load。
    while (!head.compare_exchange_weak(node->next, node,
                                       std::memory_order_release,
                                       std::memory_order_relaxed)) {
    }
}

int main() {
    constexpr int kThreads = 8;
    constexpr int kPerThread = 100'000;

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([t] {
            for (int i = 0; i < kPerThread; ++i) {
                push(t * kPerThread + i + 1);
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    std::uint64_t count = 0;
    std::uint64_t sum = 0;
    for (Node* p = head.load(std::memory_order_acquire); p != nullptr; p = p->next) {
        ++count;
        sum += static_cast<std::uint64_t>(p->value);
    }
    const std::uint64_t n = static_cast<std::uint64_t>(kThreads) * kPerThread;
    const std::uint64_t expected = n * (n + 1) / 2;

    std::printf("push 次数: %llu\n", static_cast<unsigned long long>(n));
    std::printf("遍历节点数: %llu\n", static_cast<unsigned long long>(count));
    std::printf("值的和与闭式解一致: %s\n", sum == expected ? "是" : "否");

    // 单线程清理，避免示例本身泄漏。
    Node* p = head.load(std::memory_order_relaxed);
    while (p != nullptr) {
        Node* next = p->next;
        delete p;
        p = next;
    }
}
