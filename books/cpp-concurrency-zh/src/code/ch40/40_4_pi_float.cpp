// mode: run
// flags: -ltbb
// tsan: no（TBB 运行时未插桩，会误报）
// output: varies
// expect: serial=3.1415926535
// expect: parallel=3.1415926535
// expect: ulp_diff=[-0-9]+
// expect: same=0

#include <bit>
#include <cstdint>
#include <cstdio>
#include <execution>
#include <numeric>
#include <vector>

int main() {
    constexpr std::size_t n = 8'000'000;
    std::vector<double> terms(n);
    for (std::size_t i = 0; i < n; ++i) {
        const double x = (static_cast<double>(i) + 0.5) / static_cast<double>(n);
        terms[i] = 4.0 / (1.0 + x * x);
    }

    const double serial = std::accumulate(terms.begin(), terms.end(), 0.0) /
                          static_cast<double>(n);
    const double parallel =
        std::reduce(std::execution::par, terms.begin(), terms.end(), 0.0) /
        static_cast<double>(n);

    const auto serial_bits = std::bit_cast<std::uint64_t>(serial);
    const auto parallel_bits = std::bit_cast<std::uint64_t>(parallel);
    const std::int64_t diff = static_cast<std::int64_t>(parallel_bits) -
                              static_cast<std::int64_t>(serial_bits);

    std::printf("serial=%.17g\n", serial);
    std::printf("parallel=%.17g\n", parallel);
    std::printf("ulp_diff=%lld\n", static_cast<long long>(diff));
    std::printf("same=%d\n", serial == parallel ? 1 : 0);
}
