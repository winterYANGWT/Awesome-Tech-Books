// mode: crash
// sanitize: address
// flags: -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer
// expect: heap-use-after-free
// 两个 latch 把读线程卡在“读完 head、还没读 next”之间。
#include <atomic>
#include <cstdio>
#include <latch>
#include <thread>

struct Node {
    int data;
    Node* next;
};

std::atomic<Node*> head{nullptr};
std::latch reader_loaded{1};
std::latch writer_done{1};

int main() {
    Node* b = new Node{2, nullptr};
    Node* a = new Node{1, b};
    head.store(a);

    std::thread reader([] {
        Node* old_head = head.load();
        std::printf("读线程：old_head = %d\n", old_head->data);
        reader_loaded.count_down();
        writer_done.wait();           // 等写线程弹出并释放 old_head
        Node* next = old_head->next;  // 释放后使用
        std::printf("读线程：next = %d\n", next ? next->data : -1);
        if (next) {
            head.compare_exchange_strong(old_head, next);
        }
    });

    reader_loaded.wait();
    Node* old_head = head.load();
    Node* next = old_head->next;
    if (head.compare_exchange_strong(old_head, next)) {
        std::printf("写线程：弹出 %d 并 delete\n", old_head->data);
        delete old_head;
    }
    writer_done.count_down();

    reader.join();
    delete head.load();
    return 0;
}
