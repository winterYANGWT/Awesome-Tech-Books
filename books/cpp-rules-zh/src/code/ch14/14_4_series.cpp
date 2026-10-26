// 第 14.4 节：小类型加自由函数，替代一个什么都做的类
#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <span>
#include <string>
#include <vector>

struct Reading {
    int kmh;
    int lane;
};

// 只负责一件事：保存一串读数。没有统计、格式化、告警。
class Series {
public:
    void add(Reading r) { readings_.push_back(r); }

    [[nodiscard]] std::span<const Reading> readings() const { return readings_; }
    [[nodiscard]] std::size_t size() const { return readings_.size(); }

private:
    std::vector<Reading> readings_;
};

// 下面每个函数只依赖 Series 的公开抽象，不碰它的 vector。
double meanSpeed(std::span<const Reading> rs) {
    if (rs.empty()) return 0.0;
    int sum = 0;
    for (const Reading& r : rs) sum += r.kmh;
    return static_cast<double>(sum) / rs.size();
}

int maxSpeed(std::span<const Reading> rs) {
    int m = 0;
    for (const Reading& r : rs) m = r.kmh > m ? r.kmh : m;
    return m;
}

std::size_t countFasterThan(std::span<const Reading> rs, int limit) {
    return static_cast<std::size_t>(
        std::count_if(rs.begin(), rs.end(), [limit](const Reading& r) { return r.kmh > limit; }));
}

std::string toCsv(std::span<const Reading> rs) {
    std::string out;
    for (const Reading& r : rs) {
        if (!out.empty()) out += ',';
        out += std::to_string(r.kmh);
    }
    return out;
}

// 另一个小类型：只表达“哪条车道”这一个判断。
class LaneFilter {
public:
    explicit LaneFilter(int lane) : lane_(lane) {}
    bool operator()(const Reading& r) const { return r.lane == lane_; }

private:
    int lane_;
};

int main() {
    Series s;
    for (Reading r : {Reading{72, 1}, Reading{80, 2}, Reading{95, 2},
                      Reading{75, 1}, Reading{75, 2}}) {
        s.add(r);
    }

    auto rs = s.readings();
    std::cout << "n=" << rs.size()
              << " mean=" << std::fixed << std::setprecision(1) << meanSpeed(rs)
              << " max=" << maxSpeed(rs)
              << " fast=" << countFasterThan(rs, 75) << "\n";
    std::cout << "csv=" << toCsv(rs) << "\n";
    std::cout << "lane2=" << std::ranges::count_if(rs, LaneFilter{2}) << "\n";
}
