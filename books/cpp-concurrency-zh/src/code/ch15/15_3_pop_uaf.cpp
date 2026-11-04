// mode: crash
// flags: -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer
// expect: heap-use-after-free
// expect: 线程 B: 弹出并释放了节点 1
// 说明：用 latch 强制出问题的交错，保证每次运行都复现。
#include <atomic>
#include <cstdio>
#include <latch>
#include <thread>

struct Node {
    int value;
    Node* next;
};

std::atomic<Node*> head{nullptr};
std::latch read_head{1};
std::latch freed{1};

void reader() {
    Node* old = head.load(std::memory_order_acquire);
    std::printf("线程 A: 读到 head，值是 %d\n", old->value);
    read_head.count_down();
    freed.wait();  // 等 B 把 old 释放
    Node* next = old->next;  // old 已经悬空：释放后使用
    std::printf("线程 A: next 的值是 %d\n", next->value);
    head.compare_exchange_strong(old, next);
}

void remover() {
    read_head.wait();
    Node* old = head.load(std::memory_order_acquire);
    Node* next = old->next;
    head.store(next, std::memory_order_release);
    std::printf("线程 B: 弹出并释放了节点 %d\n", old->value);
    delete old;
    freed.count_down();
}

int main() {
    head.store(new Node{1, new Node{2, nullptr}});
    std::thread a(reader);
    std::thread b(remover);
    a.join();
    b.join();
    std::printf("没有崩溃\n");
}
