// 第 35.4 节：用 std::span 把指针和长度绑成一个参数
#include <array>
#include <iostream>
#include <span>

double average(std::span<const double> values) {
    if (values.empty()) {
        return 0.0;
    }
    double total = 0;
    for (double value : values) {
        total += value;
    }
    return total / static_cast<double>(values.size());
}

int main() {
    double samples[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    std::array<double, 4> batch{10, 20, 30, 40};

    const std::span<const double> all(samples);
    std::cout << "all.size=" << all.size() << " avg=" << average(all) << '\n';
    std::cout << "batch.size=" << batch.size() << " avg=" << average(batch) << '\n';
    std::cout << "part.size=" << all.subspan(2, 3).size()
              << " avg=" << average(all.subspan(2, 3)) << '\n';
}
