// mode: run
// expect: 写入: 1000000
// expect: 读出: 1000000
// expect: 校验和与闭式解一致: 是
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <thread>

template <typename T, std::size_t Capacity>
class SpscRing {
public:
    static_assert(Capacity >= 2, "at least one slot must stay empty");

    bool push(const T& value) {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        // 读对面的进度要用 acquire：它保证消费者已经读完这个槽。
        const std::size_t head = head_.load(std::memory_order_acquire);
        if (tail - head == Capacity - 1) {
            return false;  // 留一个空槽表示满
        }
        buffer_[tail % Capacity] = value;
        // release 发布“这个槽已经写好”，消费者 acquire 读 tail 后才能读槽。
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& value) {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t tail = tail_.load(std::memory_order_acquire);
        if (head == tail) {
            return false;  // 空
        }
        value = buffer_[head % Capacity];
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

private:
    // 两个下标各占一条缓存行，避免生产者和消费者互相拖慢。
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
    alignas(64) std::array<T, Capacity> buffer_{};
};

int main() {
    constexpr std::uint64_t kCount = 1'000'000;
    SpscRing<std::uint64_t, 1024> queue;

    std::uint64_t sum = 0;
    std::uint64_t got = 0;
    std::thread consumer([&] {
        while (got < kCount) {
            std::uint64_t value = 0;
            if (queue.pop(value)) {
                sum += value;
                ++got;
            }
        }
    });

    std::thread producer([&] {
        for (std::uint64_t i = 0; i < kCount; ++i) {
            while (!queue.push(i)) {
            }
        }
    });

    producer.join();
    consumer.join();

    const std::uint64_t expected = (kCount - 1) * kCount / 2;
    std::printf("写入: %llu\n", static_cast<unsigned long long>(kCount));
    std::printf("读出: %llu\n", static_cast<unsigned long long>(got));
    std::printf("校验和: %llu\n", static_cast<unsigned long long>(sum));
    std::printf("校验和与闭式解一致: %s\n", sum == expected ? "是" : "否");
}
