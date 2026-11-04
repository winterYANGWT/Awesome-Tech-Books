// mode: crash
// sanitize: address
// flags: -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer
// expect: heap-use-after-free
// 把“计数为 1”的判断放在摘链之前，摘链之后不再检查，B 仍拿着 Y。
#include <atomic>
#include <cstdio>
#include <latch>
#include <thread>

struct Node {
    int data;
    Node* next;
    Node* retire_next;
};

std::atomic<Node*> head{nullptr};
std::atomic<unsigned> threads_in_pop{0};
std::atomic<Node*> to_be_deleted{nullptr};

std::latch a_checked{1};
std::latch b_loaded{1};
std::latch c_retired{1};
std::latch b_go{1};

void delete_list(Node* list) {
    while (list) {
        Node* next = list->retire_next;
        delete list;
        list = next;
    }
}

void retire(Node* n) {
    n->retire_next = to_be_deleted.load();
    while (!to_be_deleted.compare_exchange_weak(n->retire_next, n)) {
    }
}

int main() {
    Node* z = new Node{3, nullptr, nullptr};
    Node* y = new Node{2, z, nullptr};
    Node* w = new Node{1, y, nullptr};
    head.store(w);

    std::thread b([] {
        a_checked.wait();
        threads_in_pop.fetch_add(1);
        Node* old = head.load();
        std::printf("B：读到 old_head = %d\n", old->data);
        b_loaded.count_down();
        b_go.wait();
        Node* next = old->next;  // Y 已经被 A 删掉
        std::printf("B：next = %d\n", next ? next->data : -1);
    });

    std::thread c([] {
        b_loaded.wait();
        threads_in_pop.fetch_add(1);
        Node* old = head.load();
        while (old && !head.compare_exchange_weak(old, old->next)) {
        }
        std::printf("C：弹出 %d，挂入待删链\n", old->data);
        retire(old);
        c_retired.count_down();
    });

    // A 进入 pop，弹出 W
    threads_in_pop.fetch_add(1);
    Node* ww = head.load();
    while (ww && !head.compare_exchange_weak(ww, ww->next)) {
    }
    std::printf("A：弹出 %d\n", ww->data);
    retire(ww);

    // 判断时确实只有 A 在 pop 里
    std::printf("A：count==1，准备摘链\n");
    a_checked.count_down();
    b_loaded.wait();
    c_retired.wait();

    Node* list = to_be_deleted.exchange(nullptr);
    std::printf("A：摘下待删链，直接 delete（没有第二次检查）\n");
    delete_list(list);  // W 和 Y 一起没了
    b_go.count_down();

    b.join();
    c.join();
    return 0;
}
