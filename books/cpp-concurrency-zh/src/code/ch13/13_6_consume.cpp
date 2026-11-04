// mode: run
// expect: value = 42
#include <atomic>
#include <cstdio>

struct Node {
    int value;
};

Node node{42};
std::atomic<Node*> published{nullptr};

int main() {
    // C++26（P3475R2）弃用 memory_order_consume；GCC 15 和 Clang 21
    // 都把它当成 memory_order_acquire 处理。新代码直接写 acquire。
    published.store(&node, std::memory_order_release);
    Node* p = published.load(std::memory_order_consume);
    std::printf("value = %d\n", p->value);
}
