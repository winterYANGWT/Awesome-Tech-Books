// mode: crash
// flags: -D_GLIBCXX_ASSERTIONS
// expect: (?s)A 看到非空.*Assertion
// 每个成员函数都有自己的锁，但 empty() 和 pop() 是两次独立的调用。
// 为了稳定复现，用 latch 把 A 摆到“检查完、还没弹出”的位置。
#include <cstdio>
#include <latch>
#include <mutex>
#include <stack>
#include <thread>
#include <vector>

namespace {

std::mutex g_mutex;
std::stack<int, std::vector<int>> g_stack;

bool empty() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_stack.empty();
}

void pop(int& out) {
    std::lock_guard<std::mutex> lock(g_mutex);
    out = g_stack.top();  // 空栈时这里是未定义行为
    g_stack.pop();
}

void push(int value) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_stack.push(value);
}

}  // namespace

int main() {
    push(42);
    std::latch checked{1};
    std::latch popped{1};

    std::thread a([&] {
        const bool nonempty = !empty();
        std::printf("A 看到非空：%s\n", nonempty ? "是" : "否");
        checked.count_down();
        popped.wait();  // 等 B 把唯一的元素弹走
        int value = 0;
        pop(value);
        std::printf("A 弹出 %d\n", value);
    });

    std::thread b([&] {
        checked.wait();
        int value = 0;
        pop(value);
        std::printf("B 弹出 %d\n", value);
        popped.count_down();
    });

    a.join();
    b.join();
    return 0;
}
