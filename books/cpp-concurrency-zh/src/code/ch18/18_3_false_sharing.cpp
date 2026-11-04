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

struct Adjacent {
    std::atomic<unsigned long> a{0};
    std::atomic<unsigned long> b{0};
};

struct Separated {
    alignas(std::hardware_destructive_interference_size) std::atomic<unsigned long> a{0};
    alignas(std::hardware_destructive_interference_size) std::atomic<unsigned long> b{0};
};

double ms_since(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(
               std::chrono::steady_clock::now() - t0)
        .count();
}

template <class Layout>
double one_run(unsigned long iterations) {
    Layout counters;
    const auto t0 = std::chrono::steady_clock::now();
    std::thread t1([&] {
        for (unsigned long i = 0; i < iterations; ++i) {
            counters.a.fetch_add(1, std::memory_order_relaxed);
        }
    });
    std::thread t2([&] {
        for (unsigned long i = 0; i < iterations; ++i) {
            counters.b.fetch_add(1, std::memory_order_relaxed);
        }
    });
    t1.join();
    t2.join();
    const double ms = ms_since(t0);
    sink = counters.a.load(std::memory_order_relaxed) +
           counters.b.load(std::memory_order_relaxed);
    return ms;
}

template <class Layout>
double best_of(unsigned long iterations, int repeats) {
    double best = 1e300;
    for (int r = 0; r < repeats; ++r) {
        const double ms = one_run<Layout>(iterations);
        if (ms < best) best = ms;
    }
    return best;
}

}  // namespace

int main() {
    constexpr unsigned long iterations = 20'000'000;
    constexpr int repeats = 3;
    one_run<Adjacent>(1'000'000);   // 预热
    one_run<Separated>(1'000'000);

    std::puts("layout,ms");
    std::printf("adjacent,%.3f\n", best_of<Adjacent>(iterations, repeats));
    std::printf("separated,%.3f\n", best_of<Separated>(iterations, repeats));
}
