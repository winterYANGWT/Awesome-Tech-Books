// mode: run
// output: varies
// tsan: no（计时示例）
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <thread>

namespace {

volatile std::uint64_t sink;

std::uint64_t work(std::uint64_t n) {
    std::uint64_t sum = 0;
    for (std::uint64_t i = 0; i < n; ++i) {
        sum += (i * 2654435761u) ^ (i >> 3);
    }
    return sum;
}

}  // namespace

int main() {
    const std::clock_t cpu0 = std::clock();
    const auto wall0 = std::chrono::steady_clock::now();

    sink = work(120'000'000);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    const auto wall1 = std::chrono::steady_clock::now();
    const std::clock_t cpu1 = std::clock();

    const double cpu_ms =
        1000.0 * static_cast<double>(cpu1 - cpu0) / CLOCKS_PER_SEC;
    const double wall_ms =
        std::chrono::duration<double, std::milli>(wall1 - wall0).count();

    std::printf("clock_ms=%.1f\n", cpu_ms);
    std::printf("steady_ms=%.1f\n", wall_ms);
    std::printf("wall_minus_clock_ms=%.1f\n", wall_ms - cpu_ms);
}
