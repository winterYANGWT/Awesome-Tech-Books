// mode: run
// flags: -D_GLIBCXX_USE_TBB_PAR_BACKEND=0
// output: varies
// expect: threads=1 checksum=168526561.608

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <execution>
#include <functional>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

// 同一份源码，只把 libstdc++ 的 PSTL 后端切成串行，模拟没有 TBB 的环境。
int main() {
    std::vector<double> values(8'000'000);
    for (std::size_t i = 0; i < values.size(); ++i) {
        values[i] = static_cast<double>(i % 1000);
    }

    std::mutex seen_mutex;
    std::set<std::size_t> thread_ids;
    std::for_each(std::execution::par, values.begin(), values.end(), [&](double& x) {
        x = std::sqrt(x) + std::sin(x);
        std::lock_guard<std::mutex> lock(seen_mutex);
        thread_ids.insert(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    });

    double checksum = 0.0;
    for (double x : values) checksum += x;
    std::printf("threads=%zu checksum=%.3f\n", thread_ids.size(), checksum);
}
