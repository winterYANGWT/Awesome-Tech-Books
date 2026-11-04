// mode: hang
// expect: 仍未结束
// 两个节点的链表：正向遍历按 node1→node2 加锁，反向遍历按 node2→node1 加锁。
#include <barrier>
#include <cstdio>
#include <mutex>
#include <thread>

namespace {

struct Node {
    std::mutex mutex;
};

Node g_node1;
Node g_node2;
std::barrier g_both_ready{2};

void forward() {
    std::lock_guard<std::mutex> first(g_node1.mutex);
    std::printf("正向：锁住 node1\n");
    std::fflush(stdout);
    g_both_ready.arrive_and_wait();
    std::lock_guard<std::mutex> second(g_node2.mutex);
    std::printf("正向：锁住 node2\n");
}

void backward() {
    std::lock_guard<std::mutex> first(g_node2.mutex);
    std::printf("反向：锁住 node2\n");
    std::fflush(stdout);
    g_both_ready.arrive_and_wait();
    std::lock_guard<std::mutex> second(g_node1.mutex);
    std::printf("反向：锁住 node1\n");
}

}  // namespace

int main() {
    std::thread a(forward);
    std::thread b(backward);
    a.join();
    b.join();
    return 0;
}
