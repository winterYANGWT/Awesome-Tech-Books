// mode: hang
// expect: BusyBarrier::spin|仍未结束
// timeout: 2
// 忙等屏障只跑一轮是对的；第二轮主线程等前两个线程加上增量后清零，四个线程都等不到 4。
#include <atomic>
#include <cstdio>
#include <latch>
#include <thread>
#include <vector>

constexpr int kThreads = 4;

struct BusyBarrier {
    explicit BusyBarrier(int n) : n(n) {}
    void arrive() { count.fetch_add(1, std::memory_order_seq_cst); }
    void spin() {
        // 等所有线程到齐。没有“代”，只认这一个计数。
        while (count.load(std::memory_order_seq_cst) < n) {
            std::this_thread::yield();
        }
    }
    void wait() {
        arrive();
        spin();
    }
    int n;
    std::atomic<int> count{0};
};

BusyBarrier barrier{kThreads};
std::atomic<int> round2_order{0};
std::latch first_round_done{4};
std::latch second_go{4};
std::latch first_two_added{2};
std::latch late_go{2};

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([i] {
            barrier.wait();  // 第一轮：4 个线程都到齐，正常通过
            first_round_done.count_down();
            second_go.wait();  // 等主线程把第一轮的计数清掉
            int n = round2_order.fetch_add(1) + 1;
            if (n > 2) late_go.wait();  // 为了稳定复现，后两个等清零之后再进
            barrier.arrive();           // 第二轮：先加计数
            if (n <= 2) first_two_added.count_down();
            barrier.spin();
            std::printf("线程 %d 通过第二轮\n", i);
            std::fflush(stdout);
        });
    }

    // 第一轮结束后正常清零，再放大家进第二轮。
    first_round_done.wait();
    barrier.count.store(0, std::memory_order_seq_cst);
    second_go.count_down(4);

    // 想复用屏障：等前两个线程把第二轮的增量加上，又清一次，增量被抹掉。
    first_two_added.wait();
    barrier.count.store(0, std::memory_order_seq_cst);
    late_go.count_down(2);

    for (auto& t : threads) t.join();
    std::printf("全部通过\n");
}
