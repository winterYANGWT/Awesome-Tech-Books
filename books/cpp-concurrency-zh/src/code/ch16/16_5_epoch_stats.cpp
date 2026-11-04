// mode: run
// output: varies
// expect: bucket_us,count
// 统计节点从“逻辑删除”到“实际 delete”的延迟，输出直方图 CSV。
#include <atomic>
#include <chrono>
#include <cstdio>
#include <deque>
#include <latch>
#include <thread>
#include <vector>

constexpr int kReaders = 3;
constexpr int kNodes = 8000;
constexpr int kHoldMs = 5;

using Clock = std::chrono::steady_clock;

struct Node {
    int data;
    Node* next;
    unsigned retire_epoch;
    Clock::time_point retire_time;
    Node(int v, Node* n) : data(v), next(n), retire_epoch(0), retire_time() {}
};

struct ReaderRecord {
    std::atomic<unsigned> epoch{0};
    std::atomic<bool> active{false};
};

ReaderRecord g_readers[kReaders];
std::atomic<unsigned> g_epoch{0};
std::atomic<Node*> g_head{nullptr};
std::deque<Node*> g_retired;
std::vector<long> g_latencies;

void enter(int id) {
    g_readers[id].epoch.store(g_epoch.load(std::memory_order_acquire),
                              std::memory_order_seq_cst);
    g_readers[id].active.store(true, std::memory_order_seq_cst);
}

void leave(int id) {
    g_readers[id].active.store(false, std::memory_order_seq_cst);
}

bool grace_passed(unsigned retire_epoch) {
    for (auto& r : g_readers) {
        if (r.active.load(std::memory_order_seq_cst) &&
            r.epoch.load(std::memory_order_seq_cst) <= retire_epoch) {
            return false;
        }
    }
    return true;
}

void retire(Node* n) {
    n->retire_epoch = g_epoch.load(std::memory_order_acquire);
    n->retire_time = Clock::now();
    g_retired.push_back(n);
    g_epoch.fetch_add(1, std::memory_order_release);
}

void reclaim() {
    unsigned now = g_epoch.load(std::memory_order_acquire);
    while (!g_retired.empty()) {
        Node* n = g_retired.front();
        if (now < n->retire_epoch + 2 || !grace_passed(n->retire_epoch)) {
            break;
        }
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                      Clock::now() - n->retire_time)
                      .count();
        g_latencies.push_back(static_cast<long>(us));
        delete n;
        g_retired.pop_front();
    }
}

void reader(int id, std::latch& started, std::latch& go) {
    // 模拟一个很长的临界区：读者不放手，宽限期就不能结束
    enter(id);
    started.count_down();
    go.wait();  // 等写者就位，保证节点是在读者持旧 epoch 时被删的
    std::this_thread::sleep_for(std::chrono::milliseconds(kHoldMs));
    leave(id);
}

void writer(std::latch& started, std::latch& go) {
    started.wait();
    go.count_down();
    for (int i = 0; i < kNodes; ++i) {
        Node* old = g_head.load(std::memory_order_seq_cst);
        while (old && !g_head.compare_exchange_weak(old, old->next,
                                                    std::memory_order_seq_cst)) {
        }
        if (!old) {
            break;
        }
        retire(old);
        reclaim();
    }
}

int main() {
    Node* head = nullptr;
    for (int i = 0; i < kNodes; ++i) {
        head = new Node(i, head);
    }
    g_head.store(head, std::memory_order_release);

    std::latch started{kReaders};
    std::latch go{1};
    std::vector<std::thread> readers;
    for (int i = 0; i < kReaders; ++i) {
        readers.emplace_back(reader, i, std::ref(started), std::ref(go));
    }
    std::thread w(writer, std::ref(started), std::ref(go));
    w.join();
    for (auto& th : readers) {
        th.join();
    }

    while (!g_retired.empty()) {
        Node* n = g_retired.front();
        g_retired.pop_front();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                      Clock::now() - n->retire_time)
                      .count();
        g_latencies.push_back(static_cast<long>(us));
        delete n;
    }

    const long bounds[] = {10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000};
    long counts[11] = {};
    for (long v : g_latencies) {
        int i = 0;
        while (i < 10 && v >= bounds[i]) {
            ++i;
        }
        ++counts[i];
    }

    std::printf("bucket_us,count\n");
    long low = 0;
    for (int i = 0; i < 10; ++i) {
        std::printf("%ld-%ld,%ld\n", low, bounds[i] - 1, counts[i]);
        low = bounds[i];
    }
    std::printf(">=%ld,%ld\n", bounds[9], counts[10]);
    return 0;
}
