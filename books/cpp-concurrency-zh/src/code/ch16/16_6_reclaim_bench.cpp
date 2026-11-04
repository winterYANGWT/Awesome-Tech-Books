// mode: run
// output: varies
// cc: clang
// flags: -mcx16 -latomic
// tsan: no（计时示例，TSan 下的时间没有意义）
// 同一组 push/pop 负载，分别用风险指针、分离引用计数、epoch 回收。
#include <atomic>
#include <chrono>
#include <cstdio>
#include <deque>
#include <stdexcept>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;

constexpr int kThreads = 4;
constexpr int kIters = 100000;
constexpr int kSlots = 8;
constexpr long kReclaimThreshold = 32;

struct Result {
    double avg_us;
    long max_latency_us;
};

long microseconds_since(Clock::time_point t) {
    return static_cast<long>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - t).count());
}

void update_max(std::atomic<long>& target, long value) {
    long prev = target.load(std::memory_order_relaxed);
    while (value > prev &&
           !target.compare_exchange_weak(prev, value, std::memory_order_relaxed)) {
    }
}

// ---------------------------------------------------------------------------
// 风险指针

struct HNode {
    int data;
    HNode* next;
    HNode* retire_next;
    Clock::time_point retire_time;
    HNode(int v, HNode* n) : data(v), next(n), retire_next(nullptr), retire_time() {}
};

struct HazardSlot {
    std::atomic<bool> used{false};
    std::atomic<HNode*> ptr{nullptr};
};

HazardSlot h_slots[kSlots];

struct HpOwner {
    HazardSlot* slot = nullptr;
    HpOwner() {
        for (auto& s : h_slots) {
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
    void set(HNode* p) { slot->ptr.store(p, std::memory_order_seq_cst); }
    void clear() { slot->ptr.store(nullptr, std::memory_order_seq_cst); }
};

thread_local HpOwner h_owner;

std::atomic<HNode*> h_head{nullptr};
std::atomic<HNode*> h_pending{nullptr};
std::atomic<long> h_pending_count{0};
std::atomic<long> h_max_latency{0};
std::atomic_flag h_reclaiming = ATOMIC_FLAG_INIT;

void h_push(int v) {
    HNode* n = new HNode(v, h_head.load(std::memory_order_seq_cst));
    while (!h_head.compare_exchange_weak(n->next, n, std::memory_order_seq_cst)) {
    }
}

HNode* h_protect() {
    HNode* p = h_head.load(std::memory_order_seq_cst);
    while (p) {
        h_owner.set(p);
        if (h_head.load(std::memory_order_seq_cst) == p) {
            return p;
        }
        h_owner.clear();
        p = h_head.load(std::memory_order_seq_cst);
    }
    h_owner.clear();
    return nullptr;
}

bool h_hazardous(HNode* n) {
    for (auto& s : h_slots) {
        if (s.used.load(std::memory_order_acquire) &&
            s.ptr.load(std::memory_order_seq_cst) == n) {
            return true;
        }
    }
    return false;
}

void h_reclaim() {
    if (h_reclaiming.test_and_set(std::memory_order_acquire)) {
        return;
    }
    HNode* list = h_pending.exchange(nullptr, std::memory_order_seq_cst);
    h_pending_count.store(0, std::memory_order_seq_cst);
    HNode* keep = nullptr;
    while (list) {
        HNode* n = list;
        list = n->retire_next;
        if (h_hazardous(n)) {
            n->retire_next = keep;
            keep = n;
        } else {
            update_max(h_max_latency, microseconds_since(n->retire_time));
            delete n;
        }
    }
    while (keep) {
        HNode* n = keep;
        keep = n->retire_next;
        n->retire_next = h_pending.load(std::memory_order_seq_cst);
        while (!h_pending.compare_exchange_weak(n->retire_next, n,
                                                std::memory_order_seq_cst)) {
        }
        h_pending_count.fetch_add(1, std::memory_order_seq_cst);
    }
    h_reclaiming.clear(std::memory_order_release);
}

void h_retire(HNode* n) {
    n->retire_time = Clock::now();
    n->retire_next = h_pending.load(std::memory_order_seq_cst);
    while (!h_pending.compare_exchange_weak(n->retire_next, n,
                                            std::memory_order_seq_cst)) {
    }
    long c = h_pending_count.fetch_add(1, std::memory_order_seq_cst) + 1;
    if (c >= kReclaimThreshold) {
        h_reclaim();
    }
}

bool h_pop(int& out) {
    for (;;) {
        HNode* p = h_protect();
        if (!p) {
            return false;
        }
        HNode* next = p->next;
        if (h_head.compare_exchange_weak(p, next, std::memory_order_seq_cst)) {
            h_owner.clear();
            out = p->data;
            h_retire(p);
            return true;
        }
        h_owner.clear();
    }
}

void h_reset() {
    h_head.store(nullptr);
    h_pending.store(nullptr);
    h_pending_count.store(0);
    h_max_latency.store(0);
    for (auto& s : h_slots) {
        s.ptr.store(nullptr);
        s.used.store(false);
    }
}

// ---------------------------------------------------------------------------
// epoch

struct ENode {
    int data;
    ENode* next;
    unsigned retire_epoch;
    Clock::time_point retire_time;
    ENode(int v, ENode* n) : data(v), next(n), retire_epoch(0), retire_time() {}
};

struct ERecord {
    std::atomic<unsigned> epoch{0};
    std::atomic<bool> active{false};
};

ERecord e_records[kThreads];
std::atomic<unsigned> e_epoch{0};
std::atomic<ENode*> e_head{nullptr};
std::deque<ENode*> e_retired[kThreads];
std::atomic<long> e_max_latency{0};

unsigned e_enter(int id) {
    unsigned e = e_epoch.load(std::memory_order_acquire);
    e_records[id].epoch.store(e, std::memory_order_relaxed);
    e_records[id].active.store(true, std::memory_order_release);
    return e;
}

void e_leave(int id) {
    e_records[id].active.store(false, std::memory_order_release);
}

bool e_grace(unsigned retire_epoch) {
    for (auto& r : e_records) {
        if (r.active.load(std::memory_order_acquire) &&
            r.epoch.load(std::memory_order_relaxed) <= retire_epoch) {
            return false;
        }
    }
    return true;
}

void e_delete(ENode* n) {
    update_max(e_max_latency, microseconds_since(n->retire_time));
    delete n;
}

void e_reclaim(int id) {
    unsigned now = e_epoch.load(std::memory_order_acquire);
    while (!e_retired[id].empty()) {
        ENode* n = e_retired[id].front();
        if (now < n->retire_epoch + 2 || !e_grace(n->retire_epoch)) {
            break;
        }
        e_delete(n);
        e_retired[id].pop_front();
    }
}

void e_retire(int id, ENode* n, unsigned retire_epoch) {
    n->retire_epoch = retire_epoch;
    n->retire_time = Clock::now();
    e_retired[id].push_back(n);
    e_epoch.fetch_add(1, std::memory_order_release);
    if (e_retired[id].size() >= static_cast<size_t>(kReclaimThreshold)) {
        e_reclaim(id);
    }
}

void e_push(int v) {
    ENode* n = new ENode(v, e_head.load(std::memory_order_relaxed));
    while (!e_head.compare_exchange_weak(n->next, n, std::memory_order_release,
                                         std::memory_order_relaxed)) {
    }
}

bool e_pop(int id, int& out) {
    unsigned retire_epoch = e_enter(id);
    ENode* p = e_head.load(std::memory_order_acquire);
    while (p && !e_head.compare_exchange_weak(p, p->next, std::memory_order_acq_rel,
                                              std::memory_order_relaxed)) {
    }
    e_leave(id);
    if (!p) {
        return false;
    }
    out = p->data;
    e_retire(id, p, retire_epoch);
    return true;
}

void e_reset() {
    e_head.store(nullptr);
    e_epoch.store(0);
    e_max_latency.store(0);
    for (auto& r : e_records) {
        r.active.store(false);
        r.epoch.store(0);
    }
    for (auto& q : e_retired) {
        q.clear();
    }
}

// ---------------------------------------------------------------------------
// 分离引用计数

struct RNode;

struct RCounted {
    int external_count;
    RNode* ptr;
};

static_assert(sizeof(RCounted) == 16, "需要 16 字节");

struct RNode {
    std::atomic<int> internal_count;
    RCounted next;
    int data;
    std::atomic<long long> retire_ns;
    explicit RNode(int v)
        : internal_count(0), next{0, nullptr}, data(v), retire_ns(0) {}
};

std::atomic<RCounted> r_head{{0, nullptr}};
std::atomic<long> r_max_latency{0};

long long now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               Clock::now().time_since_epoch())
        .count();
}

void r_delete(RNode* n) {
    long long start = n->retire_ns.load(std::memory_order_relaxed);
    if (start != 0) {
        update_max(r_max_latency,
                   static_cast<long>((now_ns() - start) / 1000));
    }
    delete n;
}

void r_increase(RCounted& old) {
    RCounted next;
    do {
        next = old;
        if (next.external_count == 2147483647) {
            throw std::overflow_error("external_count 溢出");
        }
        ++next.external_count;
    } while (!r_head.compare_exchange_weak(old, next));
    old.external_count = next.external_count;
}

void r_push(int v) {
    RCounted n;
    n.ptr = new RNode(v);
    n.external_count = 1;
    n.ptr->next = r_head.load();
    while (!r_head.compare_exchange_weak(n.ptr->next, n)) {
    }
}

bool r_pop(int& out) {
    RCounted old = r_head.load();
    for (;;) {
        r_increase(old);
        RNode* p = old.ptr;
        if (!p) {
            return false;
        }
        if (r_head.compare_exchange_strong(old, p->next)) {
            out = p->data;
            p->retire_ns.store(now_ns(), std::memory_order_relaxed);
            int const increase = old.external_count - 2;
            int const previous = p->internal_count.fetch_add(increase);
            if (previous == -increase) {
                r_delete(p);
            }
            return true;
        }
        if (p->internal_count.fetch_sub(1) == 1) {
            r_delete(p);
        }
    }
}

void r_reset() {
    r_head.store(RCounted{0, nullptr});
    r_max_latency.store(0);
}

// ---------------------------------------------------------------------------
// 三个基准

Result run_hazard() {
    h_reset();
    auto start = Clock::now();
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([] {
            for (int i = 0; i < kIters; ++i) {
                h_push(i);
                int v = 0;
                while (!h_pop(v)) {
                    std::this_thread::yield();
                }
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    int v = 0;
    while (h_pop(v)) {
    }
    h_reclaim();
    for (HNode* n = h_pending.exchange(nullptr); n;) {
        HNode* next = n->retire_next;
        update_max(h_max_latency, microseconds_since(n->retire_time));
        delete n;
        n = next;
    }
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count();
    return {static_cast<double>(us) / (kThreads * kIters), h_max_latency.load()};
}

Result run_epoch() {
    e_reset();
    auto start = Clock::now();
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([t] {
            for (int i = 0; i < kIters; ++i) {
                e_push(i);
                int v = 0;
                while (!e_pop(t, v)) {
                    std::this_thread::yield();
                }
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    for (int id = 0; id < kThreads; ++id) {
        while (!e_retired[id].empty()) {
            e_delete(e_retired[id].front());
            e_retired[id].pop_front();
        }
    }
    for (ENode* n = e_head.exchange(nullptr); n;) {
        ENode* next = n->next;
        n->retire_time = Clock::now();
        e_delete(n);
        n = next;
    }
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count();
    return {static_cast<double>(us) / (kThreads * kIters), e_max_latency.load()};
}

Result run_refcount() {
    r_reset();
    auto start = Clock::now();
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([] {
            for (int i = 0; i < kIters; ++i) {
                r_push(i);
                int v = 0;
                while (!r_pop(v)) {
                    std::this_thread::yield();
                }
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    int v = 0;
    while (r_pop(v)) {
    }
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count();
    return {static_cast<double>(us) / (kThreads * kIters), r_max_latency.load()};
}

int main() {
    Result hazard = run_hazard();
    Result refcount = run_refcount();
    Result epoch = run_epoch();

    // 平均开销和最大回收延迟相差几个数量级，画不到同一根线性轴上；
    // CSV 只放平均开销，尾延迟由 16.5 的直方图看。
    std::printf("scheme,avg_us\n");
    std::printf("hazard,%.2f\n", hazard.avg_us);
    std::printf("refcount,%.2f\n", refcount.avg_us);
    std::printf("epoch,%.2f\n", epoch.avg_us);
    return 0;
}
