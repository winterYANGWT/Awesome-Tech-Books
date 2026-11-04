// mode: run
// expect: push: 800000
// expect: pop: 800000
// expect: 弹出值的和与闭式解一致: 是
// expect: 栈为空: 是
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
    // 成功：release 发布新节点和它的 data；失败：relaxed 只重读。
    while (!head.compare_exchange_weak(node->next, node,
                                       std::memory_order_release,
                                       std::memory_order_relaxed)) {
    }
}

Node* pop() {
    for (;;) {
        // 每轮重新 acquire 读 head，失败路径不拿回写的 old 去读 next。
        Node* old = head.load(std::memory_order_acquire);
        if (old == nullptr) {
            return nullptr;
        }
        Node* next = old->next;
        if (head.compare_exchange_weak(old, next,
                                       std::memory_order_acquire,
                                       std::memory_order_relaxed)) {
            return old;
        }
    }
}

int main() {
    constexpr int kThreads = 8;
    constexpr int kPerThread = 100'000;
    constexpr int kTotal = kThreads * kPerThread;

    std::vector<std::uint64_t> sums(kThreads, 0);
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([t, &sums] {
            for (int i = 0; i < kPerThread; ++i) {
                push(t * kPerThread + i + 1);
            }
            for (int i = 0; i < kPerThread; ++i) {
                Node* node = nullptr;
                do {
                    node = pop();
                } while (node == nullptr);
                sums[t] += static_cast<std::uint64_t>(node->value);
                // 节点仍然不释放：回收留给第 16 章。
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    std::uint64_t sum = 0;
    for (std::uint64_t s : sums) {
        sum += s;
    }
    const std::uint64_t expected =
        static_cast<std::uint64_t>(kTotal) * (kTotal + 1) / 2;

    std::printf("push: %d\n", kTotal);
    std::printf("pop: %d\n", kTotal);
    std::printf("弹出值的和与闭式解一致: %s\n", sum == expected ? "是" : "否");
    std::printf("栈为空: %s\n", head.load(std::memory_order_acquire) == nullptr ? "是" : "否");
}
