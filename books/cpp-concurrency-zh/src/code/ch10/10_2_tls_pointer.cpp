// mode: run
// output: varies
// expect: ready=1\nvalues=42 43 44 45
#include <array>
#include <atomic>
#include <cstdio>
#include <thread>

// 全局指针把某个线程的 thread_local 数组地址带到主线程。
std::atomic<int*> g_buf{nullptr};
std::atomic<bool> g_ready{false};

void worker() {
    thread_local std::array<int, 4> data{42, 43, 44, 45};
    g_buf.store(data.data(), std::memory_order_release);
    g_ready.store(true, std::memory_order_release);
}

int main() {
    std::thread t(worker);
    t.join();  // 汇入返回时，线程函数已经返回，thread_local 对象已经析构
    int* p = g_buf.load(std::memory_order_acquire);
    std::printf("ready=%d\n", g_ready.load(std::memory_order_acquire) ? 1 : 0);
    // 读取已析构对象是未定义行为；本机恰好还留着旧值，所以没有崩。
    std::printf("values=%d %d %d %d\n", p[0], p[1], p[2], p[3]);
}
