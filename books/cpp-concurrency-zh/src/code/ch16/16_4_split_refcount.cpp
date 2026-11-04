// mode: run
// cc: clang
// flags: -mcx16 -latomic
// 分离引用计数：外部计数在 counted_node_ptr 里，内部计数在节点里。
#include <atomic>
#include <climits>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

struct Node;

struct counted_node_ptr {
    int external_count;
    Node* ptr;
};

static_assert(sizeof(counted_node_ptr) == 16, "需要 16 字节才能用双字 CAS");
static_assert(std::is_trivially_copyable_v<counted_node_ptr>);

struct Node {
    std::atomic<int> internal_count;
    counted_node_ptr next;
    std::shared_ptr<int> data;
    explicit Node(int v)
        : internal_count(0), next{0, nullptr}, data(std::make_shared<int>(v)) {}
};

std::atomic<counted_node_ptr> head{{0, nullptr}};
std::atomic<long> pushed{0};
std::atomic<long> popped{0};

void increase_head_count(counted_node_ptr& old) {
    counted_node_ptr new_counter;
    do {
        new_counter = old;
        if (new_counter.external_count == INT_MAX) {
            throw std::overflow_error("external_count 溢出");
        }
        ++new_counter.external_count;
    } while (!head.compare_exchange_weak(old, new_counter));
    old.external_count = new_counter.external_count;
}

void push(int v) {
    counted_node_ptr new_node;
    new_node.ptr = new Node(v);
    new_node.external_count = 1;
    new_node.ptr->next = head.load();
    while (!head.compare_exchange_weak(new_node.ptr->next, new_node)) {
    }
    pushed.fetch_add(1, std::memory_order_relaxed);
}

std::shared_ptr<int> pop() {
    counted_node_ptr old_head = head.load();
    for (;;) {
        increase_head_count(old_head);
        Node* ptr = old_head.ptr;
        if (!ptr) {
            return nullptr;
        }
        if (head.compare_exchange_strong(old_head, ptr->next)) {
            std::shared_ptr<int> result;
            result.swap(ptr->data);

            int const count_increase = old_head.external_count - 2;
            int const previous = ptr->internal_count.fetch_add(count_increase);
            if (previous == -count_increase) {
                delete ptr;
            }
            popped.fetch_add(1, std::memory_order_relaxed);
            return result;
        }
        if (ptr->internal_count.fetch_sub(1) == 1) {
            delete ptr;
        }
    }
}

int main() {
    constexpr int kThreads = 8;
    constexpr int kIters = 50000;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([] {
            for (int i = 0; i < kIters; ++i) {
                push(i);
            }
            for (int i = 0; i < kIters; ++i) {
                while (!pop()) {
                    std::this_thread::yield();
                }
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    std::shared_ptr<int> value;
    while ((value = pop())) {
        value.reset();
    }

    std::printf("push: %ld 次, pop: %ld 次\n", pushed.load(), popped.load());
    std::printf("栈为空: %s\n", head.load().ptr == nullptr ? "是" : "否");
    std::printf("is_lock_free: %s\n", head.is_lock_free() ? "true" : "false");
    return 0;
}
