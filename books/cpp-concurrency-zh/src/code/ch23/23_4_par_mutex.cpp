// mode: run
// flags: -ltbb
// tsan: no（TBB 运行时未插桩，会误报）
// output: varies

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <execution>
#include <mutex>
#include <numeric>
#include <vector>

long checksum(const std::vector<int>& v) {
    long sum = 0;
    for (int x : v) sum += x;
    return sum;
}

double time_seq(std::size_t n) {
    std::vector<int> out;
    const auto t0 = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < n; ++i) out.push_back(static_cast<int>(i * 2 + 1));
    const auto t1 = std::chrono::steady_clock::now();
    std::printf("seq_ms=%.3f seq_size=%zu seq_checksum=%ld\n",
                std::chrono::duration<double, std::milli>(t1 - t0).count(), out.size(),
                checksum(out));
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

double time_par(std::size_t n, bool verbose = true) {
    std::vector<int> index(n);
    std::iota(index.begin(), index.end(), 0);
    std::vector<int> out;
    std::mutex m;

    const auto t0 = std::chrono::steady_clock::now();
    std::for_each(std::execution::par, index.begin(), index.end(), [&](int i) {
        std::lock_guard<std::mutex> lock(m);
        out.push_back(i * 2 + 1);
    });
    const auto t1 = std::chrono::steady_clock::now();
    if (verbose) {
        std::printf("par_ms=%.3f par_size=%zu par_checksum=%ld\n",
                    std::chrono::duration<double, std::milli>(t1 - t0).count(), out.size(),
                    checksum(out));
    }
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main() {
    constexpr std::size_t n = 200000;
    // 预热：先让 TBB 建好工作线程，再计时。
    time_par(1000, false);
    const double seq_ms = time_seq(n);
    const double par_ms = time_par(n);
    std::printf("par_slower=%d\n", par_ms > seq_ms ? 1 : 0);
}
