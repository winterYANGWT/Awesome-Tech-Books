// 第 27.7 节：把引擎范围缩小到 0..9，取模的偏差就能直接数出来
#include <array>
#include <cstddef>
#include <iostream>
#include <random>

// 一个范围很小、序列确定的引擎，只为把映射过程放大；真实引擎只是范围更大。
struct Digits {
    using result_type = unsigned;
    static constexpr result_type min() { return 0; }
    static constexpr result_type max() { return 9; }
    result_type operator()() { return next++ % 10; }
    result_type next = 0;
};

int main() {
    constexpr int draws = 4000;

    std::array<int, 4> modulo{};
    Digits d1;
    for (int i = 0; i != draws; ++i) ++modulo[d1() % 4];

    std::array<int, 4> uniform{};
    Digits d2;
    std::uniform_int_distribution<int> dist(0, 3);
    for (int i = 0; i != draws; ++i) ++uniform[dist(d2)];

    std::cout << "engine() % 4：";
    for (int n : modulo) std::cout << ' ' << n;
    std::cout << "\nuniform_int_distribution(0, 3)：";
    for (int n : uniform) std::cout << ' ' << n;
    std::cout << '\n';
}
