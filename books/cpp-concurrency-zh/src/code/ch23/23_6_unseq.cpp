// mode: run
// flags: -ltbb

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <execution>
#include <numeric>
#include <set>
#include <thread>
#include <vector>

int main() {
    constexpr std::size_t n = 1000000;
    std::vector<std::uint32_t> v(n);
    for (std::size_t i = 0; i < n; ++i) v[i] = static_cast<std::uint32_t>(i);

    // 用下标访问原容器，不依赖元素地址就是 v 里的对象。
    std::vector<std::size_t> index(n);
    std::iota(index.begin(), index.end(), 0);
    std::vector<std::thread::id> seen(n);
    std::for_each(std::execution::unseq, index.begin(), index.end(), [&](std::size_t i) {
        seen[i] = std::this_thread::get_id();
        v[i] = v[i] * 1664525u + 1013904223u;
    });

    std::set<std::thread::id> threads(seen.begin(), seen.end());
    std::uint64_t sum = 0;
    for (std::uint32_t x : v) sum += x;
    std::printf("threads=%zu checksum=%llu\n", threads.size(),
                static_cast<unsigned long long>(sum));
}
