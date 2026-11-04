// mode: run
// output: varies
// expect: adjacent:
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <thread>

constexpr long ITER = 20'000'000;
constexpr std::size_t kLine = std::hardware_destructive_interference_size;

struct Together {
    std::atomic<long> a{0};
    std::atomic<long> b{0};
};

struct alignas(kLine) Padded {
    std::atomic<long> v{0};
};

template <typename F>
static double measure(F&& f) {
    auto t0 = std::chrono::steady_clock::now();
    f();
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main() {
    std::printf("line=%zu iters=%ld\n", kLine, ITER);

    for (int rep = 0; rep < 2; ++rep) {
        Together together;
        double ms = measure([&] {
            std::thread t1([&] {
                for (long i = 0; i < ITER; ++i)
                    together.a.fetch_add(1, std::memory_order_relaxed);
            });
            std::thread t2([&] {
                for (long i = 0; i < ITER; ++i)
                    together.b.fetch_add(1, std::memory_order_relaxed);
            });
            t1.join();
            t2.join();
        });
        if (rep == 1)
            std::printf("adjacent: ms=%.3f sum=%ld\n", ms,
                        together.a.load() + together.b.load());
    }

    for (int rep = 0; rep < 2; ++rep) {
        Padded x, y;
        double ms = measure([&] {
            std::thread t1([&] {
                for (long i = 0; i < ITER; ++i)
                    x.v.fetch_add(1, std::memory_order_relaxed);
            });
            std::thread t2([&] {
                for (long i = 0; i < ITER; ++i)
                    y.v.fetch_add(1, std::memory_order_relaxed);
            });
            t1.join();
            t2.join();
        });
        if (rep == 1)
            std::printf("padded: ms=%.3f sum=%ld\n", ms, x.v.load() + y.v.load());
    }
    return 0;
}
