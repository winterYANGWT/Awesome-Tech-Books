// 第 26.2 节：把 end() 缓存在循环外，push_back 扩容后它指向已释放的旧缓冲区
// mode: crash
// expect: heap-use-after-free
#include <cstdio>
#include <vector>

int main() {
    std::vector<int> queue;
    queue.reserve(2);
    auto stale = queue.end();  // 记住“尾”，之后拿它写入

    queue.push_back(1);
    queue.push_back(2);
    std::puts("容量已满，再追加会重新分配");
    queue.push_back(3);  // 重新分配，旧缓冲区释放，stale 悬垂

    std::puts("通过缓存的 end() 写入");
    *stale = 42;  // 写已释放的内存
    std::printf("queue[0] = %d\n", queue[0]);
}
