// mode: run
// flags: -latomic
// expect: sum 1..10000000 via atomic_ref = 50000005000000
#include <atomic>
#include <cinttypes>
#include <cstdio>
#include <thread>

struct Sixteen {
    void* p;
    unsigned long n;
};
static_assert(sizeof(Sixteen) == 16);

int main() {
    std::atomic<int> i{0};
    std::atomic<float> f{0.0f};
    std::atomic<Sixteen> big{};

    std::printf("atomic<int>:   is_lock_free=%d is_always_lock_free=%d\n",
                i.is_lock_free(), std::atomic<int>::is_always_lock_free);
    std::printf("atomic<float>: is_lock_free=%d is_always_lock_free=%d\n",
                f.is_lock_free(), std::atomic<float>::is_always_lock_free);
    std::printf("atomic<16B>:   is_lock_free=%d is_always_lock_free=%d\n",
                big.is_lock_free(), std::atomic<Sixteen>::is_always_lock_free);

    f.fetch_add(1.5f);
    f.fetch_add(2.25f);
    f.fetch_sub(0.75f);
    std::printf("atomic<float> fetch_add/sub = %.2f\n", f.load());

    alignas(std::atomic_ref<int>::required_alignment) int plain = 0;
    {
        std::atomic_ref<int> r(plain);
        r.fetch_add(2, std::memory_order_relaxed);
        r.fetch_add(3, std::memory_order_relaxed);
    }
    std::printf("atomic_ref<int> = %d, required_alignment = %zu\n",
                plain, std::atomic_ref<int>::required_alignment);

    // 第 12 章的 sum：每个线程先算私有部分，最后用 atomic_ref 加到同一个 total。
    constexpr std::uint64_t N = 10000000;
    alignas(std::atomic_ref<std::uint64_t>::required_alignment)
        std::uint64_t total = 0;
    auto worker = [&](std::uint64_t lo, std::uint64_t hi) {
        std::uint64_t partial = 0;
        for (std::uint64_t x = lo; x <= hi; ++x) partial += x;
        std::atomic_ref<std::uint64_t> r(total);
        r.fetch_add(partial, std::memory_order_relaxed);
    };
    const std::uint64_t quarter = N / 4;
    std::thread t1(worker, 1, quarter);
    std::thread t2(worker, quarter + 1, 2 * quarter);
    std::thread t3(worker, 2 * quarter + 1, 3 * quarter);
    std::thread t4(worker, 3 * quarter + 1, N);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    const std::uint64_t expected = N * (N + 1) / 2;
    std::printf("sum 1..%" PRIu64 " via atomic_ref = %" PRIu64
                " (expected %" PRIu64 ")\n", N, total, expected);
}
