// mode: run
// output: varies
// expect: std::thread 执行流
#include <cstdio>
#include <thread>
#include <vector>

int main() {
    // ANCHOR: core
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([i] {
            std::printf("std::thread 执行流 %d/4\n", i);
        });
    }
    for (auto& t : threads) t.join();
    // ANCHOR_END: core
}
