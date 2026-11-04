// mode: run
// output: varies
// tsan: no（计时示例，TSan 会改变指令开销）
// timeout: 60
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

struct Node {
    int value;
    Node* next;
};

template <std::memory_order PushOrder, std::memory_order PopOrder>
class Stack {
public:
    void push(Node* node, int value) {
        node->value = value;
        node->next = head_.load(std::memory_order_relaxed);
        while (!head_.compare_exchange_weak(node->next, node, PushOrder,
                                            std::memory_order_relaxed)) {
        }
    }

    Node* pop() {
        for (;;) {
            Node* old = head_.load(PopOrder);
            if (old == nullptr) {
                return nullptr;
            }
            Node* next = old->next;
            if (head_.compare_exchange_weak(old, next, PopOrder,
                                            std::memory_order_relaxed)) {
                return old;
            }
        }
    }

private:
    std::atomic<Node*> head_{nullptr};
};

using Clock = std::chrono::steady_clock;
volatile std::uint64_t sink = 0;

template <typename StackType>
double run_once(int nthreads, int per_thread, std::vector<Node>& nodes) {
    StackType stack;
    std::uint64_t sum = 0;
    auto start = Clock::now();
    std::vector<std::thread> threads;
    for (int t = 0; t < nthreads; ++t) {
        threads.emplace_back([&, t] {
            Node* base = nodes.data() + static_cast<std::size_t>(t) * per_thread;
            for (int i = 0; i < per_thread; ++i) {
                stack.push(&base[i], i);
            }
            for (int i = 0; i < per_thread; ++i) {
                Node* node = nullptr;
                do {
                    node = stack.pop();
                } while (node == nullptr);
                sum += static_cast<std::uint64_t>(node->value);
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    auto stop = Clock::now();
    sink = sum;
    return std::chrono::duration<double, std::milli>(stop - start).count();
}

template <typename StackType>
double median_of(int nthreads, int per_thread, std::vector<Node>& nodes) {
    std::vector<double> samples;
    for (int r = 0; r < 3; ++r) {
        samples.push_back(run_once<StackType>(nthreads, per_thread, nodes));
    }
    std::sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];
}

int main() {
    constexpr int kThreads = 8;
    constexpr int kPerThread = 200'000;
    std::vector<Node> nodes(static_cast<std::size_t>(kThreads) * kPerThread);

    using SeqCst = Stack<std::memory_order_seq_cst, std::memory_order_seq_cst>;
    using AcqRel = Stack<std::memory_order_release, std::memory_order_acquire>;

    // 预热：先付掉页错误和线程创建的开销。
    run_once<SeqCst>(kThreads, 5'000, nodes);
    run_once<AcqRel>(kThreads, 5'000, nodes);

    const double seq = median_of<SeqCst>(kThreads, kPerThread, nodes);
    const double rel = median_of<AcqRel>(kThreads, kPerThread, nodes);
    std::printf("version,ms\n");
    std::printf("seq_cst,%.3f\n", seq);
    std::printf("release_acquire,%.3f\n", rel);
}
