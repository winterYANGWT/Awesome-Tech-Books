// mode: run
// expect: 分配节点: 1000000
// expect: 释放节点: 0
// expect: 仍然占用: 1000000
#include <atomic>
#include <cstdio>
#include <thread>
#include <vector>

struct Node {
    int value;
    Node* next;
};

std::atomic<Node*> head{nullptr};
std::atomic<long> allocated{0};
std::atomic<long> popped{0};

void push(int value) {
    Node* node = new Node{value, head.load(std::memory_order_relaxed)};
    while (!head.compare_exchange_weak(node->next, node,
                                       std::memory_order_release,
                                       std::memory_order_relaxed)) {
    }
    allocated.fetch_add(1, std::memory_order_relaxed);
}

// 第一版：弹出成功但不 delete。程序正确，内存一直涨。
Node* pop() {
    for (;;) {
        // 每轮重新 acquire 读 head：失败路径不拿回写的 old 去读 next。
        Node* old = head.load(std::memory_order_acquire);
        if (old == nullptr) {
            return nullptr;
        }
        Node* next = old->next;
        if (head.compare_exchange_weak(old, next,
                                       std::memory_order_acquire,
                                       std::memory_order_relaxed)) {
            popped.fetch_add(1, std::memory_order_relaxed);
            return old;
        }
    }
}

int main() {
    constexpr int kThreads = 4;
    constexpr int kPerThread = 250'000;

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([t] {
            for (int i = 0; i < kPerThread; ++i) {
                push(t * kPerThread + i + 1);
            }
            for (int i = 0; i < kPerThread; ++i) {
                while (pop() == nullptr) {
                }
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    long a = allocated.load(std::memory_order_relaxed);
    long p = popped.load(std::memory_order_relaxed);
    std::printf("分配节点: %ld\n", a);
    std::printf("释放节点: 0\n");
    std::printf("已经弹出但未释放: %ld\n", p);
    std::printf("仍然占用: %ld\n", a);
}
