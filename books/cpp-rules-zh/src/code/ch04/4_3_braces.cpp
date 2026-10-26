// 第 4.3 节：花括号能消歧、拒绝窄化，但遇到 initializer_list 会优先选它
#include <initializer_list>
#include <iostream>
#include <vector>

struct Config {
    Config(int retries, bool verbose) {
        std::cout << "Config(int, bool): retries=" << retries
                  << ", verbose=" << verbose << "\n";
    }
    Config(int retries, double ratio) {
        std::cout << "Config(int, double): retries=" << retries
                  << ", ratio=" << ratio << "\n";
    }
};

struct Reading {
    Reading() { std::cout << "Reading() 默认构造\n"; }
    Reading(int count, bool valid) {
        std::cout << "Reading(int, bool): count=" << count
                  << ", valid=" << valid << "\n";
    }
    Reading(std::initializer_list<long double> values) {
        std::cout << "Reading(initializer_list<long double>): "
                  << values.size() << " 个值\n";
    }
};

struct Point {
    int x;
    int y;
};

int main() {
    std::cout << "--- 没有 initializer_list：两种括号等价 ---\n";
    Config a(10, true);
    Config b{10, true};
    Config c(10, 2.5);
    Config d{10, 2.5};

    std::cout << "--- 有 initializer_list：花括号优先选它 ---\n";
    Reading e(10, true);    // 圆括号：精确匹配 (int, bool)
    Reading f{10, true};    // 花括号：两个实参转成 long double，选列表构造函数

    std::cout << "--- 空花括号表示“没有实参” ---\n";
    Reading g{};            // 默认构造，不是空 initializer_list
    Reading h({});          // 把空花括号当成一个实参，选列表构造函数

    std::cout << "--- vector 的两种含义 ---\n";
    std::vector<int> sized(4, 9);    // 4 个元素，每个都是 9
    std::vector<int> listed{4, 9};   // 2 个元素：4 和 9
    std::cout << "sized: " << sized.size() << " 个元素，sized[0]=" << sized[0] << "\n";
    std::cout << "listed: " << listed.size() << " 个元素，listed[0]=" << listed[0]
              << "，listed[1]=" << listed[1] << "\n";

    std::cout << "--- C++20：聚合体也能用圆括号 ---\n";
    Point p(1, 2);
    Point q{3, 4};
    std::cout << "p = (" << p.x << "," << p.y << ")，q = (" << q.x << "," << q.y << ")\n";
}
