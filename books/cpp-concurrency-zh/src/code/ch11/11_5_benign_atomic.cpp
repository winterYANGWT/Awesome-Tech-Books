// mode: run
// output: varies
// expect: 后写的是 1 的次数 = [0-9]+
// 两个线程都只做原子的 store：没有数据竞争，但谁最后写仍由调度决定。
#include <atomic>
#include <iostream>
#include <latch>
#include <thread>

int main() {
    constexpr int kRounds = 200;
    std::atomic<int> value{0};
    int ones = 0;
    int twos = 0;

    for (int round = 0; round < kRounds; ++round) {
        value.store(0);
        std::latch start{3};
        std::thread a([&] {
            start.arrive_and_wait();
            value.store(1);
        });
        std::thread b([&] {
            start.arrive_and_wait();
            value.store(2);
        });
        start.arrive_and_wait();
        a.join();
        b.join();
        if (value.load() == 1) {
            ++ones;
        } else {
            ++twos;
        }
    }

    std::cout << "后写的是 1 的次数 = " << ones << '\n'
              << "后写的是 2 的次数 = " << twos << '\n';
}
