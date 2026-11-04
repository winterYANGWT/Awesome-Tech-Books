// mode: run
// sanitize: address
// expect: 存活节点: 0 个
// 每个线程发布自己在读的地址；删除线程扫描所有槽，没人指向才 delete。
#include <atomic>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include <vector>

struct Node {
    int data;
    Node* next;
    Node* retire_next;  // 待回收链专用，读者只读 next
    explicit Node(int v) : data(v), next(nullptr), retire_next(nullptr) {}
};

constexpr int kSlots = 16;
constexpr long kReclaimThreshold = 64;

struct HazardSlot {
    std::atomic<bool> used{false};
    std::atomic<Node*> ptr{nullptr};
};

HazardSlot g_slots[kSlots];

struct HpOwner {
    HazardSlot* slot = nullptr;

    HpOwner() {
        for (auto& s : g_slots) {
            bool expected = false;
            if (s.used.compare_exchange_strong(expected, true)) {
                slot = &s;
                break;
            }
        }
        if (!slot) {
            throw std::runtime_error("风险指针槽表满");
        }
    }
    ~HpOwner() {
        if (slot) {
            slot->ptr.store(nullptr, std::memory_order_release);
            slot->used.store(false, std::memory_order_release);
        }
    }
    HpOwner(const HpOwner&) = delete;
    HpOwner& operator=(const HpOwner&) = delete;

    void set(Node* p) { slot->ptr.store(p, std::memory_order_seq_cst); }
    void clear() { slot->ptr.store(nullptr, std::memory_order_seq_cst); }
};

thread_local HpOwner t_hp;

std::atomic<Node*> g_head{nullptr};
std::atomic<Node*> g_pending{nullptr};
std::atomic<long> g_pending_count{0};
std::atomic<long> g_reclaimed{0};
std::atomic<long> g_allocated{0};
std::atomic_flag g_reclaiming = ATOMIC_FLAG_INIT;

void push(int v) {
    Node* n = new Node(v);
    g_allocated.fetch_add(1, std::memory_order_relaxed);
    n->next = g_head.load(std::memory_order_seq_cst);
    while (!g_head.compare_exchange_weak(n->next, n, std::memory_order_seq_cst)) {
    }
}

Node* protect_head() {
    Node* p = g_head.load(std::memory_order_seq_cst);
    while (p) {
        t_hp.set(p);  // 先发布
        if (g_head.load(std::memory_order_seq_cst) == p) {
            return p;  // 再确认 head 没变，才能解引用
        }
        t_hp.clear();
        p = g_head.load(std::memory_order_seq_cst);
    }
    t_hp.clear();
    return nullptr;
}

bool hazardous(Node* n) {
    for (auto& s : g_slots) {
        if (s.used.load(std::memory_order_acquire) &&
            s.ptr.load(std::memory_order_seq_cst) == n) {
            return true;
        }
    }
    return false;
}

void reclaim() {
    if (g_reclaiming.test_and_set(std::memory_order_acquire)) {
        return;  // 已经有线程在回收
    }
    Node* list = g_pending.exchange(nullptr, std::memory_order_seq_cst);
    g_pending_count.store(0, std::memory_order_seq_cst);
    Node* keep = nullptr;
    while (list) {
        Node* n = list;
        list = n->retire_next;
        if (hazardous(n)) {
            n->retire_next = keep;
            keep = n;
        } else {
            delete n;
            g_reclaimed.fetch_add(1, std::memory_order_relaxed);
        }
    }
    while (keep) {
        Node* n = keep;
        keep = n->retire_next;
        n->retire_next = g_pending.load(std::memory_order_seq_cst);
        while (!g_pending.compare_exchange_weak(n->retire_next, n,
                                                std::memory_order_seq_cst)) {
        }
        g_pending_count.fetch_add(1, std::memory_order_seq_cst);
    }
    g_reclaiming.clear(std::memory_order_release);
}

void retire(Node* n) {
    n->retire_next = g_pending.load(std::memory_order_seq_cst);
    while (!g_pending.compare_exchange_weak(n->retire_next, n,
                                            std::memory_order_seq_cst)) {
    }
    long c = g_pending_count.fetch_add(1, std::memory_order_seq_cst) + 1;
    if (c >= kReclaimThreshold) {
        reclaim();
    }
}

bool pop(int& out) {
    for (;;) {
        Node* p = protect_head();
        if (!p) {
            return false;
        }
        Node* next = p->next;
        if (g_head.compare_exchange_weak(p, next, std::memory_order_seq_cst)) {
            t_hp.clear();
            out = p->data;
            retire(p);
            return true;
        }
        t_hp.clear();
    }
}

int main() {
    constexpr int kThreads = 8;
    constexpr int kIters = 50000;

    std::atomic<long> pushed{0};
    std::atomic<long> popped{0};
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < kIters; ++i) {
                push(i);
                pushed.fetch_add(1, std::memory_order_relaxed);
            }
            for (int i = 0; i < kIters; ++i) {
                int v = 0;
                while (!pop(v)) {
                    std::this_thread::yield();
                }
                popped.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    // 线程都走了，风险指针槽已归还；把栈和待回收链清空
    int v = 0;
    while (pop(v)) {
        popped.fetch_add(1, std::memory_order_relaxed);
    }
    reclaim();
    Node* list = g_pending.exchange(nullptr);
    while (list) {
        Node* n = list;
        list = n->retire_next;
        delete n;
        g_reclaimed.fetch_add(1, std::memory_order_relaxed);
    }

    long alive = g_allocated.load() - g_reclaimed.load();
    std::printf("push: %ld 次, pop: %ld 次\n", pushed.load(), popped.load());
    std::printf("回收: %ld 个\n", g_reclaimed.load());
    std::printf("存活节点: %ld 个\n", alive);
    std::printf("槽表大小: %d 个\n", kSlots);
    return 0;
}
