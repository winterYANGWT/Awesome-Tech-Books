// mode: run
// expect: 线程 1: CAS 成功
// expect: 线程 1: 弹出的值 = 99（期望 1）
// expect: 最终 head 指向 B
// 说明：用 latch 和固定节点池强制出 ABA 的交错，保证每次运行都复现。
#include <atomic>
#include <cstdio>
#include <latch>
#include <thread>

struct Node {
    int value;
    Node* next;
};

// 固定池：弹出后把节点还回池子，下一次分配会拿到同一个地址，
// 这正是 ABA 里“地址相同、历史不同”的来源。
Node pool[2];
Node* free_list = nullptr;

Node* allocate() {
    Node* n = free_list;
    free_list = free_list->next;
    return n;
}

void release(Node* n) {
    n->next = free_list;
    free_list = n;
}

std::atomic<Node*> head{nullptr};
std::latch t1_read{1};
std::latch t2_done{1};

void thread1() {
    Node* old = head.load(std::memory_order_acquire);  // A
    Node* next = old->next;                            // B
    std::printf("线程 1: old = A（值 %d），next = B（值 %d）\n",
                old->value, next->value);
    t1_read.count_down();
    t2_done.wait();

    // 它只比较地址：head 从 A 变成 B 又变回 A'，地址仍是 A。
    bool ok = head.compare_exchange_strong(old, next,
                                           std::memory_order_acq_rel,
                                           std::memory_order_acquire);
    std::printf("线程 1: CAS %s\n", ok ? "成功" : "失败");
    if (ok) {
        std::printf("线程 1: 弹出的值 = %d（期望 1）\n", old->value);
    }
}

void thread2() {
    t1_read.wait();

    Node* a = head.load(std::memory_order_acquire);  // A
    head.store(a->next, std::memory_order_release);  // head = B
    Node* b = head.load(std::memory_order_acquire);  // B
    head.store(b->next, std::memory_order_release);  // head = null

    release(b);
    release(a);  // 池子按后进先出，下一次分配拿到 A 的地址

    Node* a2 = allocate();
    a2->value = 99;
    a2->next = nullptr;
    head.store(a2, std::memory_order_release);  // head = A'
    std::printf("线程 2: 弹出 A、B，压入 A'（地址同 A，值 99）\n");
    t2_done.count_down();
}

int main() {
    // 池子初始状态：A 在栈顶，B 在 A 后面。
    pool[0] = Node{1, &pool[1]};
    pool[1] = Node{2, nullptr};
    free_list = nullptr;
    // 先把 A、B 从池子里“分配”出去，再压栈。
    free_list = &pool[0];
    Node* a = allocate();
    free_list = &pool[1];
    Node* b = allocate();
    a->next = b;
    b->next = nullptr;
    head.store(a, std::memory_order_release);

    std::thread t1(thread1);
    std::thread t2(thread2);
    t1.join();
    t2.join();

    Node* final_head = head.load(std::memory_order_acquire);
    std::printf("最终 head 指向 B（值 %d）\n", final_head->value);
}
