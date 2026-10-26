// 第 5.2 节：auto 避免类型捷径，也让闭包保持自己的类型
// flags: -Wno-range-loop-construct
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

struct Meter {
    static int copies;
    std::string name;

    explicit Meter(std::string n) : name(std::move(n)) {}
    Meter(const Meter& other) : name(other.name) { ++copies; }
    Meter(Meter&&) noexcept = default;
    Meter& operator=(const Meter&) = default;
    Meter& operator=(Meter&&) noexcept = default;
};

int Meter::copies = 0;

int main() {
    std::unordered_map<std::string, Meter> meters;
    meters.emplace("kitchen", Meter("kitchen"));
    meters.emplace("garage", Meter("garage"));
    meters.emplace("garden", Meter("garden"));

    // 元素类型是 pair<const string, Meter>，写成 pair<string, Meter> 会
    // 每轮都构造一个临时 pair，把 Meter 也复制一遍。
    int wrong = 0;
    for (const std::pair<std::string, Meter>& entry : meters) {
        wrong += static_cast<int>(entry.second.name.size());
    }
    std::cout << "手写错误类型：Meter 复制 " << Meter::copies << " 次\n";

    Meter::copies = 0;
    int right = 0;
    for (const auto& entry : meters) {
        right += static_cast<int>(entry.second.name.size());
    }
    std::cout << "auto& 绑定：Meter 复制 " << Meter::copies << " 次\n";

    // v.size() 的真实类型是 size_type，不是 unsigned
    std::vector<int> numbers(7, 1);
    auto sz = numbers.size();
    static_assert(std::is_same_v<decltype(sz), std::vector<int>::size_type>);
    std::cout << "sz = " << sz << "，第一轮长度和 = " << wrong
              << "，第二轮长度和 = " << right << "\n";

    // 闭包的真实类型只有编译器知道；auto 保存它，std::function 则做类型擦除
    auto add_tax = [rate = 0.2](double amount) { return amount * (1 + rate); };
    std::function<double(double)> erased = add_tax;
    std::cout << "闭包大小 = " << sizeof(add_tax)
              << "，std::function 大小 = " << sizeof(erased) << "\n";
    std::cout << "add_tax(100) = " << add_tax(100.0)
              << "，erased(100) = " << erased(100.0) << "\n";
}
