// mode: crash
// sanitize: address
// flags: -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer
// expect: heap-use-after-free
// 读线程发布了风险指针，但发布之前 head 已经变了；它没有重读 head。
#include <atomic>
#include <cstdio>
#include <latch>
#include <thread>

struct Node {
    int data;
    Node* next;
};

std::atomic<Node*> head{nullptr};
std::atomic<Node*> hazard{nullptr};

std::latch reader_loaded{1};
std::latch writer_done{1};

int main() {
    Node* b = new Node{2, nullptr};
    Node* a = new Node{1, b};
    head.store(a);

    std::thread reader([] {
        Node* p = head.load();
        std::printf("读线程：读到 p = %d\n", p->data);
        reader_loaded.count_down();
        writer_done.wait();

        hazard.store(p);  // 发布太晚：A 已经不在栈里了
        // 少了 if (head.load() == p) 的重读确认
        Node* next = p->next;  // 释放后使用
        std::printf("读线程：next = %d\n", next ? next->data : -1);
    });

    reader_loaded.wait();
    Node* old = head.load();
    Node* next = old->next;
    if (head.compare_exchange_strong(old, next)) {
        std::printf("写线程：弹出 %d 并 delete\n", old->data);
        delete old;
    }
    writer_done.count_down();

    reader.join();
    delete head.load();
    return 0;
}
