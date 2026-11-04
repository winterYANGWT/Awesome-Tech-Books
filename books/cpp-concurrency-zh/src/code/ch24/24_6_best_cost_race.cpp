// mode: race
// expect: data race
#include <cstdio>
#include <thread>

// 旧写法：用 volatile 阻止编译器把读提到循环外，但它不建立同步。
volatile int best_cost = 1'000'000;

int main() {
    long long seen = 0;
    std::thread searcher([&] {
        for (int i = 0; i < 2'000'000; ++i) {
            int b = best_cost;          // 无锁读
            if (b < 1'000'000) seen += b;
        }
    });
    std::thread updater([] {
        for (int i = 0; i < 1000; ++i)
            best_cost = 1000 + i;       // 无锁写
    });
    searcher.join();
    updater.join();
    std::printf("done seen=%lld\n", seen);
    return 0;
}
