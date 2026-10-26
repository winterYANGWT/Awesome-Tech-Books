// 第 29.2 节：结果几乎一定被反复请求时，提前维护或用缓存摊薄成本
#include <cstddef>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

// 维护运行中的统计数据：插入时更新，查询 O(1)。
class RunningStats {
public:
    void add(double value) {
        ++count_;
        sum_ += value;
        if (value < min_) min_ = value;
        if (value > max_) max_ = value;
        ++visits_;   // 每个元素只在插入时看一次
    }

    std::size_t count() const { return count_; }
    double avg() const { return count_ ? sum_ / static_cast<double>(count_) : 0.0; }
    double min() const { return min_; }
    double max() const { return max_; }
    std::size_t visits() const { return visits_; }

private:
    std::size_t count_ = 0;
    double sum_ = 0.0;
    double min_ = std::numeric_limits<double>::max();
    double max_ = std::numeric_limits<double>::lowest();
    std::size_t visits_ = 0;
};

// 每次都重新扫描：查询多少次，就把整个数组看多少遍。
class RescanStats {
public:
    void add(double value) { values_.push_back(value); }

    double avg() const {
        double sum = 0.0;
        for (double v : values_) { sum += v; ++visits_; }
        return values_.empty() ? 0.0 : sum / static_cast<double>(values_.size());
    }

    double max() const {
        double result = std::numeric_limits<double>::lowest();
        for (double v : values_) { ++visits_; if (v > result) result = v; }
        return result;
    }

    std::size_t visits() const { return visits_; }

private:
    std::vector<double> values_;
    mutable std::size_t visits_ = 0;
};

// 昂贵的外部查询：同一个 key 反复问，缓存能把数据库访问压到不同 key 的个数。
namespace db {
inline int lookups = 0;
std::string lookup(const std::string& employee) {
    ++lookups;
    return "desk-" + std::to_string(std::hash<std::string>{}(employee) % 1000);
}
}  // namespace db

std::string desk_number(const std::string& employee) {
    static std::unordered_map<std::string, std::string> cache;
    if (auto it = cache.find(employee); it != cache.end()) {
        return it->second;                    // 命中缓存，不碰数据库
    }
    auto [it, inserted] = cache.try_emplace(employee, db::lookup(employee));
    (void)inserted;
    return it->second;
}

int main() {
    constexpr int elements = 10'000;
    constexpr int queries = 1000;

    RunningStats running;
    for (int i = 0; i < elements; ++i) running.add(i % 97);
    for (int i = 0; i < queries; ++i) {
        volatile double sink = running.avg() + running.min() + running.max();
        (void)sink;
    }
    std::cout << "running: 元素访问 " << running.visits()
              << " 次（插入时一次），查询 " << queries << " 次\n";

    RescanStats rescan;
    for (int i = 0; i < elements; ++i) rescan.add(i % 97);
    for (int i = 0; i < queries; ++i) {
        volatile double sink = rescan.avg() + rescan.max();
        (void)sink;
    }
    std::cout << "rescan:  元素访问 " << rescan.visits() << " 次\n";

    // 50 个不同员工，每个被问 200 次：缓存把 10000 次查询压成 50 次。
    db::lookups = 0;
    for (int i = 0; i < 10'000; ++i) {
        volatile std::string result = desk_number("employee-" + std::to_string(i % 50));
        (void)result;
    }
    std::cout << "缓存命中: 10000 次请求，数据库访问 " << db::lookups << " 次\n";
}
