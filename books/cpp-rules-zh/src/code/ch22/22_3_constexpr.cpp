// 第 22.3 节：constexpr 对象保证编译期初值，函数按实参决定求值时机
#include <array>
#include <iostream>
#include <type_traits>

constexpr int square(int x) { return x * x; }

// C++20：同一个函数在常量表达式里走一条路，在运行期走另一条。
constexpr int scaled(int x) {
    if (std::is_constant_evaluated()) return x * 2;
    return x * 3;
}

// C++20：consteval 强制每次调用都在编译期求值。
consteval int checked_capacity(int n) {
    return n > 0 ? n : throw "capacity must be positive";
}

struct Reading {
    int value;
    constexpr Reading(int v) : value(v) {}
    constexpr int doubled() const { return value * 2; }
};

constexpr Reading sample{21};
static_assert(sample.doubled() == 42);

constexpr int compile_time = scaled(10);              // 编译期求值：20
constinit int global_capacity = checked_capacity(8);  // 静态初始化，但不是 const

int main() {
    std::array<int, square(3)> cells{};  // 函数调用出现在模板实参里
    std::cout << "square(3) 作数组长度：" << cells.size() << '\n';

    int runtime = 10;
    std::cout << "scaled 编译期 = " << compile_time
              << "，运行期 = " << scaled(runtime) << '\n';

    global_capacity += 1;  // constinit 不要求 const，值可以改
    std::cout << "global_capacity = " << global_capacity << '\n';

    constexpr auto table = [] {  // C++17 constexpr lambda，C++20 里可直接初始化 constexpr 变量
        std::array<int, 4> t{};
        for (int i = 0; i < 4; ++i) t[i] = square(i);
        return t;
    }();
    static_assert(table[3] == 9);
    std::cout << "constexpr lambda：" << table[0] << ' ' << table[1] << ' '
              << table[2] << ' ' << table[3] << '\n';

    constexpr Reading r{5};
    std::cout << "literal type：" << r.doubled() << '\n';
}
