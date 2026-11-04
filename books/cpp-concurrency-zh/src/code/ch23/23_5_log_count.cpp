// mode: run
// flags: -ltbb
// tsan: no（TBB 运行时未插桩，会误报）

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <execution>
#include <numeric>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct log_info {
    std::string page;
    long count = 0;
};

log_info parse(const std::string& s) {
    const auto a = s.find(' ');
    const auto b = s.find(' ', a + 1);
    log_info out;
    out.page = s.substr(a + 1, b - a - 1);
    out.count = s.substr(b + 1) == "200" ? 1 : 0;
    return out;
}

struct combine_visits {
    using map = std::unordered_map<std::string, long>;

    map operator()(map a, map b) const {
        if (a.size() < b.size()) std::swap(a, b);
        for (const auto& [page, count] : b) a[page] += count;
        return a;
    }
    map operator()(map a, const log_info& line) const {
        if (line.count != 0) a[line.page] += line.count;
        return a;
    }
    map operator()(const log_info& line, map a) const { return (*this)(std::move(a), line); }
    map operator()(const log_info& x, const log_info& y) const {
        map m;
        if (x.count != 0) m[x.page] += x.count;
        if (y.count != 0) m[y.page] += y.count;
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

int main() {
    const auto log = make_log(200000);
    const auto counts = std::transform_reduce(std::execution::par, log.begin(), log.end(),
                                              combine_visits::map{}, combine_visits{}, parse);

    std::vector<std::pair<std::string, long>> ranked(counts.begin(), counts.end());
    std::sort(ranked.begin(), ranked.end(), [](const auto& x, const auto& y) {
        return x.second != y.second ? x.second > y.second : x.first < y.first;
    });

    long total = 0;
    for (const auto& [page, count] : ranked) total += count;

    std::printf("pages=%zu total=%ld\n", ranked.size(), total);
    std::puts("top5:");
    for (std::size_t i = 0; i < 5 && i < ranked.size(); ++i) {
        std::printf("%s %ld\n", ranked[i].first.c_str(), ranked[i].second);
    }
}
