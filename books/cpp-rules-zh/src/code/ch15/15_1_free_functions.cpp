// 第 15.1 节：便利操作写成非成员非友元，类只留最小公开接口
#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

namespace monitoring {

class SensorLog {
public:
    explicit SensorLog(std::string name) : name_(std::move(name)) {}

    void record(double value) { samples_.push_back(value); }

    const std::string& name() const { return name_; }
    std::size_t size() const { return samples_.size(); }
    bool empty() const { return samples_.empty(); }
    double operator[](std::size_t i) const { return samples_[i]; }
    auto begin() const { return samples_.begin(); }
    auto end() const { return samples_.end(); }

private:
    std::string name_;
    std::vector<double> samples_;
};

// 非成员非友元：只读 SensorLog 的公开接口，不增加能碰私有数据的函数数量。
template <class Range>
double average_of(const Range& r) {
    using std::begin;
    using std::end;
    auto first = begin(r);
    auto last = end(r);
    if (first == last) {
        return 0.0;
    }
    double sum = std::accumulate(first, last, 0.0);
    return sum / static_cast<double>(std::distance(first, last));
}

double average(const SensorLog& log) { return average_of(log); }

double peak(const SensorLog& log) {
    if (log.empty()) {
        return 0.0;
    }
    return *std::max_element(log.begin(), log.end());
}

}  // namespace monitoring

// 客户扩展：不改 SensorLog，也不继承，只在同一命名空间里再加一个自由函数。
namespace monitoring {

std::size_t count_above(const SensorLog& log, double limit) {
    return static_cast<std::size_t>(std::count_if(
        log.begin(), log.end(), [limit](double value) { return value > limit; }));
}

}  // namespace monitoring

int main() {
    monitoring::SensorLog log("boiler-1");
    log.record(71.2);
    log.record(73.8);
    log.record(69.5);
    log.record(75.1);

    std::cout << std::fixed << std::setprecision(1);
    std::cout << log.name() << ": " << log.size() << " 个读数\n";
    std::cout << "average(log)       = " << monitoring::average(log) << '\n';
    std::cout << "peak(log)          = " << monitoring::peak(log) << '\n';
    std::cout << "count_above(log,70)= " << monitoring::count_above(log, 70.0) << '\n';

    // 同一个模板也服务 vector<double>：成员函数做不到这一点。
    std::vector<double> raw{1.0, 2.0, 6.0};
    std::cout << "average_of(vector) = " << monitoring::average_of(raw) << '\n';
    std::cout << "average_of(log)    = " << monitoring::average_of(log) << '\n';
}
