// mode: run
// output: varies
// tsan: no（计时示例，重复次数多）
// timeout: 60
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <new>
#include <thread>

namespace {

volatile std::uint64_t sink;
constexpr std::size_t line = std::hardware_destructive_interference_size;

struct Aligned {
    alignas(line) std::atomic<unsigned long> v{0};
};

struct Padded {
    std::atomic<unsigned long> v{0};
    char pad[line - sizeof(std::atomic<unsigned long>)];
};

static_assert(sizeof(Aligned) == line);
static_assert(sizeof(Padded) == line);

double ms_since(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(
               std::chrono::steady_clock::now() - t0)
        .count();
}

template <class Counter>
double one_run(unsigned long iterations, Counter& a, Counter& b) {
    const auto t0 = std::chrono::steady_clock::now();
    std::thread t1([&] {
        for (unsigned long i = 0; i < iterations; ++i) {
            a.v.fetch_add(1, std::memory_order_relaxed);
        }
    });
    std::thread t2([&] {
        for (unsigned long i = 0; i < iterations; ++i) {
            b.v.fetch_add(1, std::memory_order_relaxed);
        }
    });
    t1.join();
    t2.join();
    const double ms = ms_since(t0);
    sink = a.v.load(std::memory_order_relaxed) +
           b.v.load(std::memory_order_relaxed);
    return ms;
}

}  // namespace

int main() {
    constexpr unsigned long iterations = 25'000'000;
    constexpr int repeats = 3;

    Aligned aligned[2];
    alignas(line) Padded padded[2];

    std::puts("method,ms");
    double best_aligned = 1e300;
    double best_padded = 1e300;
    for (int r = 0; r < repeats; ++r) {
        const double a = one_run(iterations, aligned[0], aligned[1]);
        const double p = one_run(iterations, padded[0], padded[1]);
        if (a < best_aligned) best_aligned = a;
        if (p < best_padded) best_padded = p;
    }
    std::printf("alignas,%.3f\n", best_aligned);
    std::printf("padding,%.3f\n", best_padded);
}
