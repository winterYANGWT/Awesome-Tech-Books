// mode: run
// expect: 主线程 counter: 0[\s\S]*线程 3 的值: 3[\s\S]*地址互不相同: true
#include <array>
#include <cstdio>
#include <thread>
#include <vector>

// 命名空间作用域的 thread_local：每个线程一份，主线程也有自己的一份。
thread_local int counter = 0;

int main() {
    constexpr int kThreads = 4;
    std::array<const int*, kThreads> addrs{};
    std::array<int, kThreads> values{};

    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([i, &addrs, &values] {
            for (int n = 0; n < 3; ++n) ++counter;
            addrs[i] = &counter;
            values[i] = counter;
        });
    }
    for (auto& t : threads) t.join();

    bool distinct = true;
    for (int i = 0; i < kThreads; ++i)
        for (int j = i + 1; j < kThreads; ++j)
            if (addrs[i] == addrs[j]) distinct = false;

    std::printf("主线程 counter: %d\n", counter);
    for (int i = 0; i < kThreads; ++i)
        std::printf("线程 %d 的值: %d\n", i, values[i]);
    std::printf("地址互不相同: %s\n", distinct ? "true" : "false");
}
