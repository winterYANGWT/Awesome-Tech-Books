// mode: run
// expect: 8 字节 tagged pointer 无锁: 是
// expect: 第一次 CAS 失败
// expect: 重试后弹出的值 = 99
// expect: 最终栈为空: 是
// 说明：用 latch 和固定节点池强制出与 15_5_aba.cpp 相同的交错。
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <latch>
#include <thread>

struct Node {
    int value;
    Node* next;
};

// 本机 x86-64（4 级页表）的用户态地址不超过 48 位，高 16 位可以放版本计数。
// 换平台或换用 5 级页表时要先确认指针位宽，或者改用 16 字节的 {指针, tag} 结构体。
constexpr int kTagBits = 16;
constexpr std::uint64_t kPtrMask = (std::uint64_t{1} << 48) - 1;

std::uint64_t pack(Node* p, std::uint16_t tag) {
    const auto raw = reinterpret_cast<std::uintptr_t>(p);
    assert((raw & ~kPtrMask) == 0);
    return (static_cast<std::uint64_t>(tag) << 48) | raw;
}

Node* unpack_ptr(std::uint64_t v) {
    return reinterpret_cast<Node*>(v & kPtrMask);
}

std::uint16_t unpack_tag(std::uint64_t v) {
    return static_cast<std::uint16_t>(v >> 48);
}

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

std::atomic<std::uint64_t> head{0};
std::latch t1_read{1};
std::latch t2_done{1};

Node* ordinary_pop() {
    for (;;) {
        std::uint64_t old = head.load(std::memory_order_acquire);
        Node* ptr = unpack_ptr(old);
        if (ptr == nullptr) {
            return nullptr;
        }
        Node* next = ptr->next;
        if (head.compare_exchange_weak(old, pack(next, unpack_tag(old) + 1),
                                       std::memory_order_acquire,
                                       std::memory_order_relaxed)) {
            return ptr;
        }
    }
}

void thread1() {
    std::uint64_t old = head.load(std::memory_order_acquire);
    Node* old_ptr = unpack_ptr(old);
    Node* next = old_ptr->next;
    std::printf("线程 1: 读到 A（tag %u），next = B\n",
                static_cast<unsigned>(unpack_tag(old)));
    t1_read.count_down();
    t2_done.wait();

    // 和 ABA 版一样只比较一个机器字，但这一次高 16 位的 tag 也参与比较。
    bool ok = head.compare_exchange_strong(
        old, pack(next, unpack_tag(old) + 1),
        std::memory_order_acq_rel, std::memory_order_acquire);
    std::printf("线程 1: 第一次 CAS %s（head 的 tag 现在是 %u）\n",
                ok ? "成功" : "失败",
                static_cast<unsigned>(unpack_tag(old)));
    if (!ok) {
        Node* got = ordinary_pop();
        std::printf("线程 1: 重试后弹出的值 = %d\n", got->value);
    }
}

void thread2() {
    t1_read.wait();

    std::uint64_t cur = head.load(std::memory_order_acquire);
    Node* a = unpack_ptr(cur);
    head.store(pack(a->next, unpack_tag(cur) + 1),
               std::memory_order_release);  // head = B
    cur = head.load(std::memory_order_acquire);
    Node* b = unpack_ptr(cur);
    head.store(pack(b->next, unpack_tag(cur) + 1),
               std::memory_order_release);  // head = null
    cur = head.load(std::memory_order_acquire);  // 空指针也带着 tag

    release(b);
    release(a);

    Node* a2 = allocate();
    a2->value = 99;
    a2->next = nullptr;
    head.store(pack(a2, unpack_tag(cur) + 1),
               std::memory_order_release);  // head = A'，tag 又变了
    std::printf("线程 2: 弹出 A、B，压入地址相同的 A'（值 99）\n");
    t2_done.count_down();
}

int main() {
    std::printf("8 字节 tagged pointer 无锁: %s\n",
                head.is_lock_free() ? "是" : "否");

    pool[0] = Node{1, &pool[1]};
    pool[1] = Node{2, nullptr};
    free_list = &pool[0];
    Node* a = allocate();
    free_list = &pool[1];
    Node* b = allocate();
    a->next = b;
    b->next = nullptr;
    head.store(pack(a, 1), std::memory_order_release);

    std::thread t1(thread1);
    std::thread t2(thread2);
    t1.join();
    t2.join();

    // 空指针也可以带非零 tag，所以要看指针部分，不能直接和 0 比。
    std::printf("最终栈为空: %s\n",
                unpack_ptr(head.load(std::memory_order_acquire)) == nullptr ? "是" : "否");
}
