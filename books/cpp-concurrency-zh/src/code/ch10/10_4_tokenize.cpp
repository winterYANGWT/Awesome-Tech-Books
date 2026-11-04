// mode: run
// expect: 第 0 行: 数字个数=4 和=100[\s\S]*第 3 行: 数字个数=3 和=600
#include <charconv>
#include <cstdio>
#include <string_view>
#include <thread>
#include <vector>

struct Result {
    int count = 0;
    long long sum = 0;
};

// ANCHOR: parse_line
// 没有静态状态：起点是参数 line，进度是局部变量 line 本身。
Result parse_line(std::string_view line) {
    Result r;
    while (!line.empty()) {
        while (!line.empty() && line.front() == ' ') line.remove_prefix(1);
        if (line.empty()) break;
        long long value = 0;
        const auto res = std::from_chars(line.data(), line.data() + line.size(), value);
        if (res.ec != std::errc{}) break;
        ++r.count;
        r.sum += value;
        line.remove_prefix(static_cast<std::size_t>(res.ptr - line.data()));
    }
    return r;
}
// ANCHOR_END: parse_line

int main() {
    const std::vector<std::string_view> lines = {
        "10 20 30 40",
        "1 2 3 4 5 6",
        "-5 5 -10 10",
        "100 200 300",
    };
    std::vector<Result> results(lines.size());
    std::vector<std::thread> threads;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        threads.emplace_back([&, i] { results[i] = parse_line(lines[i]); });
    }
    for (auto& t : threads) t.join();
    for (std::size_t i = 0; i < lines.size(); ++i) {
        std::printf("第 %zu 行: 数字个数=%d 和=%lld\n", i, results[i].count,
                    results[i].sum);
    }
}
