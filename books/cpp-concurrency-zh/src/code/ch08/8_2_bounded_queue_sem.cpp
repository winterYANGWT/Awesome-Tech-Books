// mode: run
// output: varies
// expect: 入队 100000，出队 100000
// 容量 8 的有界队列：empty 信号量数空位，full 信号量数元素，mutex 只保护队列结构。
#include <semaphore>

#include <atomic>
#include <cstdio>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

constexpr int kCapacity = 8;
constexpr int kProducers = 4;
constexpr int kConsumers = 4;
constexpr int kPerProducer = 25000;
constexpr int kTotal = kProducers * kPerProducer;

std::counting_semaphore<kCapacity> empty_slots{kCapacity};
std::counting_semaphore<kCapacity> full_slots{0};
std::mutex queue_mutex;
std::deque<int> queue;
int max_length = 0;  // 只在持锁时改
std::atomic<long> enqueued{0};
std::atomic<long> dequeued{0};

void produce(int id) {
    for (int i = 0; i < kPerProducer; ++i) {
        empty_slots.acquire();  // 先拿到空位，再碰队列
        {
            std::lock_guard lk(queue_mutex);
            queue.push_back(id * kPerProducer + i);
            if (static_cast<int>(queue.size()) > max_length) max_length = queue.size();
            enqueued.fetch_add(1, std::memory_order_relaxed);
        }
        full_slots.release();  // 解锁之后再通知：元素已经写好
    }
}

void consume() {
    for (int i = 0; i < kTotal / kConsumers; ++i) {
        full_slots.acquire();  // 先等到元素，再碰队列
        {
            std::lock_guard lk(queue_mutex);
            queue.pop_front();
            dequeued.fetch_add(1, std::memory_order_relaxed);
        }
        empty_slots.release();
    }
}

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < kProducers; ++i) threads.emplace_back(produce, i);
    for (int i = 0; i < kConsumers; ++i) threads.emplace_back(consume);
    for (auto& t : threads) t.join();

    std::printf("入队 %ld，出队 %ld\n", enqueued.load(), dequeued.load());
    std::printf("队列最大长度 %d，容量 %d：%s\n", max_length, kCapacity,
                max_length <= kCapacity ? "没有超过容量" : "超过容量");
}
