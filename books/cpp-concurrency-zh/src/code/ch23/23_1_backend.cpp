// mode: run
// flags: -ltbb
// output: varies
// tsan: no（线程数观察示例，TSan 下过慢）

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <execution>
#include <numeric>
#include <set>
#include <thread>
#include <utility>
#include <vector>

// 每个元素只写自己的槽，避免为了记录线程号而引入共享写。
template <class Work>
std::pair<std::size_t, double> run_pass(std::size_t n, Work work) {
    std::vector<std::size_t> index(n);
    std::iota(index.begin(), index.end(), 0);
    std::vector<std::thread::id> seen(n);
    std::vector<double> out(n);

    std::for_each(std::execution::par, index.begin(), index.end(), [&](std::size_t i) {
        seen[i] = std::this_thread::get_id();
        out[i] = work(i);
    });

    std::set<std::thread::id> threads(seen.begin(), seen.end());
    return {threads.size(), std::accumulate(out.begin(), out.end(), 0.0)};
}

int main() {
    std::printf("hardware_concurrency=%u\n", std::thread::hardware_concurrency());

    // 小工作量：只做一次乘法和一次写。
    auto small = run_pass(10000, [](std::size_t i) { return static_cast<double>(i) * 0.5; });
    std::printf("small_threads=%zu small_checksum=%.3f\n", small.first, small.second);

    // 大工作量：每个元素做 1000 次 sin，让每个任务都活得够久。
    auto large = run_pass(2000000, [](std::size_t i) {
        double x = static_cast<double>(i) * 1e-6;
        for (int k = 0; k < 1000; ++k) {
            x = std::sin(x + 0.001);
        }
        return x;
    });
    std::printf("large_threads=%zu large_checksum=%.3f\n", large.first, large.second);
}
