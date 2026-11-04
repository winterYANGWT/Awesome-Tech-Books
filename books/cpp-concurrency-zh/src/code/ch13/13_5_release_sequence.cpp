// mode: run
// expect: 两个消费者累加之和: 190
#include <atomic>
#include <cstdio>
#include <thread>

constexpr int kItems = 20;
int data[kItems];                 // 生产者写、消费者读，靠释放序列同步
std::atomic<int> count{0};
std::atomic<long> total{0};

void producer() {
    for (int i = 0; i < kItems; ++i) {
        data[i] = i;
    }
    count.store(kItems, std::memory_order_release);  // 链头
}

void consumer() {
    // relaxed 只用来等链头发布；可见性由下面的 acquire 负责。
    while (count.load(std::memory_order_relaxed) == 0) {
    }

    long sum = 0;
    // 每个消费者领 10 个下标。后一个 fetch_sub 可能读到另一个
    // 消费者的读-改-写结果，它仍挂在同一条释放序列上。
    for (int taken = 0; taken < kItems / 2; ++taken) {
        int index = count.fetch_sub(1, std::memory_order_acquire);
        sum += data[index - 1];
    }
    total.fetch_add(sum, std::memory_order_relaxed);
}

int main() {
    // 消费者先启动，生产者的 join 不再替它们建立先行发生关系。
    std::thread c1(consumer), c2(consumer);
    std::thread p(producer);
    p.join();
    c1.join();
    c2.join();
    std::printf("两个消费者累加之和: %ld\n", total.load());
}
