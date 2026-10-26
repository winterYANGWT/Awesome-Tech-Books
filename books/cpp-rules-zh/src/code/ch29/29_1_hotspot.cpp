// 第 29.1 节：用计时证明“看起来贵”的函数不一定占大头
// sanitize: none
// flags: -O2
// output: varies
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;

// 重复若干次取中位数，先跑一次预热；只报数量级，不把单次数字当结论。
template <class F>
double median_ms(F f) {
    std::vector<double> samples;
    for (int round = 0; round < 7; ++round) {
        const auto start = Clock::now();
        f();
        const auto stop = Clock::now();
        samples.push_back(std::chrono::duration<double, std::milli>(stop - start).count());
    }
    std::sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];
}

int main() {
    constexpr int n = 500'000;

    // 500k 个短字符串：全部落在 SSO 缓冲区里，拷贝时不分配堆内存。
    std::vector<std::string> names;
    names.reserve(n);
    for (int i = 0; i < n; ++i) names.push_back("user" + std::to_string(i % 1000));

    std::vector<int> numbers;
    numbers.reserve(n);
    std::mt19937 rng(20261029);
    for (int i = 0; i < n; ++i) numbers.push_back(static_cast<int>(rng()));

    std::uint64_t sink = 0;

    const double copy_ms = median_ms([&] {
        std::vector<std::string> copy = names;      // 500k 次 string 拷贝
        sink += copy[n / 2].size();
    });

    const double sort_ms = median_ms([&] {
        std::vector<int> copy = numbers;
        std::sort(copy.begin(), copy.end());        // O(n log n) 次比较
        sink += static_cast<std::uint64_t>(copy[0]);
    });

    const double sum_ms = median_ms([&] {
        sink += std::accumulate(numbers.begin(), numbers.end(), std::uint64_t{0});
    });

    std::cout << "copy 500k short strings : " << copy_ms << " ms\n";
    std::cout << "sort 500k ints          : " << sort_ms << " ms\n";
    std::cout << "sum  500k ints          : " << sum_ms << " ms\n";
    std::cout << "sort/copy ratio         : " << sort_ms / copy_ms << '\n';
    std::cout << "(sink=" << sink << "，防止优化器删掉被测代码)\n";
}
