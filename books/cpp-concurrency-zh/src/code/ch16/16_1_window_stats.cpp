// mode: run
// output: varies
// expect: 窗口被其他 pop 穿过: [1-9]\d* 次
// 纯压力循环：节点不释放，只在读 next 之后、CAS 之前留一个可观测的窗口。
#include <atomic>
#include <cstdio>
#include <thread>
#include <vector>

struct Node {
    int data;
    Node* next;
    std::atomic<bool> popped{false};
};

std::atomic<Node*> head{nullptr};
std::atomic<long> popped_total{0};
std::atomic<long> crossed{0};
volatile int g_window_sink = 0;  // 让窗口里的循环不能被优化掉

void push(Node* n) {
    n->next = head.load(std::memory_order_relaxed);
    while (!head.compare_exchange_weak(n->next, n, std::memory_order_release,
                                       std::memory_order_relaxed)) {
    }
}

bool pop_once() {
    for (;;) {
        Node* old = head.load(std::memory_order_acquire);
        if (!old) {
            return false;
        }
        Node* next = old->next;
        // 人为拉长窗口：真实代码里这一段是准备 CAS 和等缓存行
        for (int i = 0; i < 64; ++i) {
            (void)g_window_sink;
        }
        if (old->popped.load(std::memory_order_relaxed)) {
            // 这个节点在窗口里被别的 pop 摘走了
            crossed.fetch_add(1, std::memory_order_relaxed);
            continue;
        }
        if (head.compare_exchange_weak(old, next, std::memory_order_acq_rel,
                                       std::memory_order_relaxed)) {
            old->popped.store(true, std::memory_order_relaxed);
            popped_total.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
    }
}

int main() {
    constexpr int kThreads = 8;
    constexpr long kNodes = 400000;

    for (long i = 0; i < kNodes; ++i) {
        push(new Node{static_cast<int>(i), nullptr});
    }

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([] {
            while (pop_once()) {
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    std::printf("pop 成功: %ld 次\n", popped_total.load());
    std::printf("窗口被其他 pop 穿过: %ld 次\n", crossed.load());
    std::printf("节点故意不释放，所以读 next 不会变成悬空指针\n");
    return 0;
}
