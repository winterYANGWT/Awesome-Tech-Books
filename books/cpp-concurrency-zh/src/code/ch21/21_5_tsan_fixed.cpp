// mode: run
// expect: counter = 200000
// 同一个负载，把计数器改成 std::atomic<int>；fetch_add 是原子的读-改-写，
// TSan 不再报竞争。这里用 relaxed：只需要原子性，不需要用它给别的数据排序。
#include <atomic>
#include <cstdio>
#include <thread>

namespace {

std::atomic<int> counter{0};

void add_many() {
    for (int i = 0; i < 100000; ++i) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
}

}  // namespace

int main() {
    std::thread a(add_many);
    std::thread b(add_many);
    a.join();
    b.join();
    std::printf("counter = %d\n", counter.load(std::memory_order_relaxed));
    return 0;
}
