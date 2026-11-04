// mode: run
// flags: -ltbb
// tsan: no（TBB 运行时未插桩，会误报）
// output: varies

#include <bit>
#include <cstdint>
#include <cstdio>
#include <execution>
#include <numeric>
#include <utility>
#include <vector>

int main() {
    constexpr std::size_t n = 5000000;

    // 整数和：accumulate 是严格左折叠，reduce 允许重排；整数加法满足结合律。
    std::vector<std::uint64_t> numbers(n);
    std::iota(numbers.begin(), numbers.end(), 1ULL);
    const std::uint64_t sum_acc = std::accumulate(numbers.begin(), numbers.end(), 0ULL);
    const std::uint64_t sum_red =
        std::reduce(std::execution::par, numbers.begin(), numbers.end(), 0ULL);

    // 浮点项：每个元素是 π 中点法的一项，加法不满足结合律。
    std::vector<double> terms(n);
    for (std::size_t i = 0; i < n; ++i) {
        const double x = (static_cast<double>(i) + 0.5) / static_cast<double>(n);
        terms[i] = 4.0 / (1.0 + x * x);
    }
    const double pi_acc = std::accumulate(terms.begin(), terms.end(), 0.0) / static_cast<double>(n);
    const double pi_red =
        std::reduce(std::execution::par, terms.begin(), terms.end(), 0.0) / static_cast<double>(n);

    // 两个 double 的位模式相减，得到相差多少个 ULP（这里只关心是否为 0）。
    const auto bits_acc = std::bit_cast<std::uint64_t>(pi_acc);
    const auto bits_red = std::bit_cast<std::uint64_t>(pi_red);

    const std::vector<int> prefix{1, 2, 3, 4, 5, 6, 7, 8};
    std::vector<int> inclusive(prefix.size());
    std::vector<int> exclusive(prefix.size());
    std::inclusive_scan(std::execution::par, prefix.begin(), prefix.end(), inclusive.begin());
    std::exclusive_scan(std::execution::par, prefix.begin(), prefix.end(), exclusive.begin(), 100);

    std::printf("sum_accumulate=%llu\n", static_cast<unsigned long long>(sum_acc));
    std::printf("sum_reduce=%llu\n", static_cast<unsigned long long>(sum_red));
    std::printf("pi_accumulate=%.17g\n", pi_acc);
    std::printf("pi_reduce=%.17g\n", pi_red);
    std::printf("pi_ulp_diff=%lld\n", static_cast<long long>(bits_acc) - static_cast<long long>(bits_red));

    std::printf("scan_inclusive=");
    for (int v : inclusive) std::printf("%d ", v);
    std::printf("\nscan_exclusive_init100=");
    for (int v : exclusive) std::printf("%d ", v);
    std::printf("\n");
}
