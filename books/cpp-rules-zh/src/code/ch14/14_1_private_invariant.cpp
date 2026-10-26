// 第 14.1 节：private 成员让访问函数同时承担校验和实现替换
#include <cassert>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <vector>

// 纯数据聚合：数据成员就是接口，全部 public，不维护任何不变式。
struct Reading {
    int kmh;
    int lane;
};
static_assert(std::is_aggregate_v<Reading>);

// 实现一：只保存累计值。维护的不变式是 count_、sum_、max_ 同进同退。
class SpeedStats {
public:
    void add(int kmh) {
        if (kmh < 0) throw std::invalid_argument("negative speed");
        ++count_;
        sum_ += kmh;
        if (kmh > max_) max_ = kmh;
        assert(invariant());
    }

    [[nodiscard]] int count() const { return count_; }
    [[nodiscard]] int max() const { return max_; }
    [[nodiscard]] double average() const {
        return count_ == 0 ? 0.0 : static_cast<double>(sum_) / count_;
    }
    [[nodiscard]] bool invariant() const {
        return count_ >= 0 && sum_ >= 0 && max_ >= 0
               && (count_ == 0) == (sum_ == 0);
    }

private:
    int count_ = 0;
    int sum_ = 0;
    int max_ = 0;
};
static_assert(!std::is_aggregate_v<SpeedStats>);

// 实现二：接口相同，内部改成保留每条记录，平均值现算。
class ReplayStats {
public:
    void add(int kmh) {
        if (kmh < 0) throw std::invalid_argument("negative speed");
        samples_.push_back(kmh);
    }

    [[nodiscard]] int count() const { return static_cast<int>(samples_.size()); }
    [[nodiscard]] int max() const {
        int m = 0;
        for (int v : samples_) m = v > m ? v : m;
        return m;
    }
    [[nodiscard]] double average() const {
        if (samples_.empty()) return 0.0;
        int sum = 0;
        for (int v : samples_) sum += v;
        return static_cast<double>(sum) / samples_.size();
    }

private:
    std::vector<int> samples_;
};

// 调用方只依赖 count/max/average 这三个名字：内部换实现不用改这里。
template <class Stats>
void report(const char* label, const Stats& s) {
    std::cout << label << ": n=" << s.count() << " max=" << s.max()
              << " avg=" << std::fixed << std::setprecision(1) << s.average() << "\n";
}

int main() {
    Reading r{.kmh = 80, .lane = 2};   // C++20 指定初始化器：纯聚合才有的写法
    std::cout << "reading " << r.kmh << " km/h lane " << r.lane << "\n";

    SpeedStats running;
    ReplayStats replay;
    for (int v : {72, 90, 81, 89}) {
        running.add(v);
        replay.add(v);
    }
    report("running", running);
    report("replay", replay);

    try {
        running.add(-1);                // 访问函数拒绝破坏不变式的输入
    } catch (const std::exception& e) {
        std::cout << "rejected: " << e.what() << "\n";
    }
    std::cout << "after rejected: n=" << running.count()
              << " invariant=" << running.invariant() << "\n";
}
