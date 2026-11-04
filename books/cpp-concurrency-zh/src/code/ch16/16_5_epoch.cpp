// mode: run
// output: varies
// expect: 写者释放: 10000 个
// expect: 栈为空: 是
// 4 个读者在 epoch 临界区里遍历，1 个写者弹出节点；宽限期过后才 delete。
#include <atomic>
#include <cstdio>
#include <deque>
#include <latch>
#include <thread>
#include <vector>

constexpr int kReaders = 4;
constexpr int kNodes = 10000;
constexpr int kTraversals = 200;

struct Node {
    int data;
    Node* next;
    unsigned retire_epoch;
    Node(int v, Node* n) : data(v), next(n), retire_epoch(0) {}
};

struct ReaderRecord {
    std::atomic<unsigned> epoch{0};
    std::atomic<bool> active{false};
};

ReaderRecord g_readers[kReaders];
std::atomic<unsigned> g_epoch{0};
std::atomic<Node*> g_head{nullptr};
std::deque<Node*> g_retired;  // 只有写者访问
long g_freed = 0;
long g_seen[kReaders] = {};

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
        delete n;
        g_retired.pop_front();
        ++g_freed;
    }
}

void reader(int id, std::latch& started) {
    long seen = 0;
    bool first = true;
    for (int t = 0; t < kTraversals; ++t) {
        enter(id);
        if (first) {
            started.count_down();
            first = false;
        }
        Node* p = g_head.load(std::memory_order_seq_cst);
        while (p) {
            seen += p->data;
            p = p->next;
        }
        leave(id);
    }
    g_seen[id] = seen;
}

void writer(std::latch& started) {
    started.wait();
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
    std::vector<std::thread> readers;
    for (int i = 0; i < kReaders; ++i) {
        readers.emplace_back(reader, i, std::ref(started));
    }
    std::thread w(writer, std::ref(started));
    w.join();
    for (auto& th : readers) {
        th.join();
    }

    // 读者都走了，剩下的旧节点可以释放
    while (!g_retired.empty()) {
        delete g_retired.front();
        g_retired.pop_front();
        ++g_freed;
    }

    std::printf("节点总数: %d 个\n", kNodes);
    std::printf("写者释放: %ld 个\n", g_freed);
    std::printf("每个读者遍历: %d 次\n", kTraversals);
    for (int i = 0; i < kReaders; ++i) {
        std::printf("读者 %d 访问计数: %ld\n", i, g_seen[i]);
    }
    std::printf("栈为空: %s\n", g_head.load() == nullptr ? "是" : "否");
    return 0;
}
