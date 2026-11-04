// mode: run
// flags: -ltbb
// output: varies
// tsan: no（计时示例，TSan 下过慢）

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <execution>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <tbb/global_control.h>

struct log_line {
    std::string page;
    bool ok = false;
};

log_line parse(const std::string& s) {
    const auto a = s.find(' ');
    const auto b = s.find(' ', a + 1);
    log_line out;
    out.page = s.substr(a + 1, b - a - 1);
    out.ok = s.substr(b + 1) == "200";
    return out;
}

struct combine_visits {
    using map = std::unordered_map<std::string, long>;
    map operator()(map a, map b) const {
        if (a.size() < b.size()) std::swap(a, b);
        for (const auto& [page, count] : b) a[page] += count;
        return a;
    }
    map operator()(map a, const log_line& line) const {
        if (line.ok) ++a[line.page];
        return a;
    }
    map operator()(const log_line& line, map a) const { return (*this)(std::move(a), line); }
    map operator()(const log_line& x, const log_line& y) const {
        map m;
        if (x.ok) ++m[x.page];
        if (y.ok) ++m[y.page];
        return m;
    }
};

std::vector<std::string> make_log(std::size_t n) {
    std::mt19937 rng(20261103);
    std::vector<std::string> log;
    log.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        const int page = static_cast<int>(rng() % 40);
        const bool ok = (rng() % 10) != 0;
        log.push_back("GET /page" + std::to_string(page) + (ok ? " 200" : " 500"));
    }
    return log;
}

template <class Policy>
std::pair<double, long> run(Policy&& policy, const std::vector<std::string>& log) {
    const auto t0 = std::chrono::steady_clock::now();
    const auto counts = std::transform_reduce(policy, log.begin(), log.end(),
                                              combine_visits::map{}, combine_visits{}, parse);
    const auto t1 = std::chrono::steady_clock::now();
    long total = 0;
    for (const auto& [page, count] : counts) total += count;
    return {std::chrono::duration<double, std::milli>(t1 - t0).count(), total};
}

// ANCHOR: bench
int main() {
    const auto log = make_log(200000);
    std::puts("mode,threads,ms,checksum");

    auto median = [](std::vector<double> v) {
        std::sort(v.begin(), v.end());
        return v[v.size() / 2];
    };

    // 预热：先跑一遍 seq，让分配器和 TBB 都进入状态。
    run(std::execution::seq, log);

    std::vector<double> seq_ms;
    long seq_checksum = 0;
    for (int repeat = 0; repeat < 3; ++repeat) {
        const auto [ms, checksum] = run(std::execution::seq, log);
        seq_ms.push_back(ms);
        seq_checksum = checksum;
    }
    std::printf("seq,1,%.3f,%ld\n", median(seq_ms), seq_checksum);

    for (int threads : {1, 2, 4, 8}) {
        // 执行策略本身没有线程数参数；这里通过 TBB 的全局上限做实验。
        tbb::global_control limit(tbb::global_control::max_allowed_parallelism,
                                  static_cast<std::size_t>(threads));
        std::vector<double> ms;
        long checksum = 0;
        for (int repeat = 0; repeat < 3; ++repeat) {
            const auto result = run(std::execution::par, log);
            ms.push_back(result.first);
            checksum = result.second;
        }
        std::printf("par,%d,%.3f,%ld\n", threads, median(ms), checksum);
    }
}
// ANCHOR_END: bench
