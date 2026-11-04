// mode: race
// expect: data race
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
        // 反例：读对面进度也用 relaxed，槽的归还就没有同步。
        const std::size_t head = head_.load(std::memory_order_relaxed);
        if (tail - head == Capacity - 1) {
            return false;  // 留一个空槽表示满
        }
        buffer_[tail % Capacity] = value;
        // 反例：发布也用 relaxed，写槽和读 tail 之间没有同步。
        tail_.store(tail + 1, std::memory_order_relaxed);
        return true;
    }

    bool pop(T& value) {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        if (head == tail) {
            return false;  // 空
        }
        value = buffer_[head % Capacity];
        head_.store(head + 1, std::memory_order_relaxed);
        return true;
    }

private:
    // 两个下标各占一条缓存行，避免生产者和消费者互相拖慢。
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
    alignas(64) std::array<T, Capacity> buffer_{};
};

int main() {
    constexpr std::uint64_t kCount = 100'000;
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
