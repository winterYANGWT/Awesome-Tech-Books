// mode: run
// output: varies
// expect: 待删链最长: [1-9]\d* 个
// expect: 总回收: 160000 个
// 8 个线程 push/pop；只有 threads_in_pop 为 1 的线程有资格回收。
#include <atomic>
#include <cstdio>
#include <memory>
#include <thread>
#include <vector>

struct Node {
    std::shared_ptr<int> data;
    Node* next;
    Node* retire_next;  // 待删链专用，避免和读者正在读的 next 冲突
    explicit Node(int v)
        : data(std::make_shared<int>(v)), next(nullptr), retire_next(nullptr) {}
};

std::atomic<Node*> head{nullptr};
std::atomic<unsigned> threads_in_pop{0};
std::atomic<Node*> to_be_deleted{nullptr};
std::atomic<long> pending{0};
std::atomic<long> max_pending{0};
std::atomic<long> reclaimed{0};

void update_max(long v) {
    long prev = max_pending.load(std::memory_order_relaxed);
    while (v > prev &&
           !max_pending.compare_exchange_weak(prev, v, std::memory_order_relaxed)) {
    }
}

void push(int v) {
    Node* n = new Node(v);
    n->next = head.load(std::memory_order_relaxed);
    while (!head.compare_exchange_weak(n->next, n, std::memory_order_release,
                                       std::memory_order_relaxed)) {
    }
}

void delete_list(Node* list) {
    while (list) {
        Node* next = list->retire_next;
        delete list;
        reclaimed.fetch_add(1, std::memory_order_relaxed);
        pending.fetch_sub(1, std::memory_order_relaxed);
        list = next;
    }
}

void push_list(Node* list) {
    Node* tail = list;
    while (tail->retire_next) {
        tail = tail->retire_next;
    }
    tail->retire_next = to_be_deleted.load(std::memory_order_relaxed);
    while (!to_be_deleted.compare_exchange_weak(tail->retire_next, list,
                                                std::memory_order_release,
                                                std::memory_order_relaxed)) {
    }
}

void add_to_list(Node* n) {
    n->retire_next = to_be_deleted.load(std::memory_order_relaxed);
    while (!to_be_deleted.compare_exchange_weak(n->retire_next, n,
                                                std::memory_order_release,
                                                std::memory_order_relaxed)) {
    }
    long p = pending.fetch_add(1, std::memory_order_relaxed) + 1;
    update_max(p);
}

void try_reclaim(Node* old) {
    if (threads_in_pop.load(std::memory_order_seq_cst) != 1) {
        add_to_list(old);
        return;
    }
    Node* list = to_be_deleted.exchange(nullptr, std::memory_order_seq_cst);
    // 摘下链之后再查一次：可能刚好有线程在摘链的瞬间进入 pop
    if (threads_in_pop.load(std::memory_order_seq_cst) == 1) {
        delete_list(list);
        delete old;  // 当前节点不在待删链上，也要一起回收
        reclaimed.fetch_add(1, std::memory_order_relaxed);
    } else {
        if (list) {
            push_list(list);
        }
        add_to_list(old);  // 有人刚进来，当前节点也不能删
    }
}

std::shared_ptr<int> pop() {
    threads_in_pop.fetch_add(1, std::memory_order_seq_cst);
    Node* old = head.load(std::memory_order_seq_cst);
    while (old && !head.compare_exchange_weak(old, old->next, std::memory_order_seq_cst)) {
    }
    std::shared_ptr<int> result;
    if (old) {
        result = std::move(old->data);
        try_reclaim(old);
    }
    threads_in_pop.fetch_sub(1, std::memory_order_seq_cst);
    return result;
}

int main() {
    constexpr int kThreads = 8;
    constexpr int kIters = 20000;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([] {
            for (int i = 0; i < kIters; ++i) {
                push(i);
                pop();
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    // 所有线程已退出，剩下的待删链可以安全清空
    delete_list(to_be_deleted.exchange(nullptr));
    std::shared_ptr<int> value;
    while ((value = pop())) {
        value.reset();
    }
    delete_list(to_be_deleted.exchange(nullptr));

    std::printf("待删链最长: %ld 个\n", max_pending.load());
    std::printf("总回收: %ld 个\n", reclaimed.load());
    return 0;
}
