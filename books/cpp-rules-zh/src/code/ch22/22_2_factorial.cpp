// 第 22.2 节：旧式模板元编程算阶乘，与 constexpr 和类型层面的元编程对照
#include <array>
#include <cstddef>
#include <iostream>
#include <utility>

// C++98 的写法：递归实例化代替循环，全特化提供终止条件。
template <unsigned N>
struct Factorial {
    static constexpr unsigned long long value =
        N * Factorial<N - 1>::value;  // C++11 前这里用 enum { value = ... }
};

template <>
struct Factorial<0> {
    static constexpr unsigned long long value = 1;
};

// C++11 的 constexpr 函数：函数体只有一条 return，用递归写循环。
constexpr unsigned long long factorial_recursive(unsigned n) {
    return n == 0 ? 1 : n * factorial_recursive(n - 1);
}

// C++14 放宽之后：局部变量和循环都可以写了。
constexpr unsigned long long factorial_loop(unsigned n) {
    unsigned long long result = 1;
    for (unsigned i = 2; i <= n; ++i) result *= i;
    return result;
}

// 类型层面的计算仍然离不开模板：把 integer_sequence 展开成数组。
template <class T, T... Is>
constexpr std::array<T, sizeof...(Is)> to_array(std::integer_sequence<T, Is...>) {
    return {{Is...}};
}

int main() {
    static_assert(Factorial<0>::value == 1);
    static_assert(Factorial<5>::value == 120);
    static_assert(factorial_recursive(6) == 720);
    static_assert(factorial_loop(6) == 720);

    std::cout << "Factorial<10>       = " << Factorial<10>::value << '\n';
    std::cout << "factorial_recursive = " << factorial_recursive(10) << '\n';
    std::cout << "factorial_loop      = " << factorial_loop(10) << '\n';

    constexpr auto squares = to_array(std::make_integer_sequence<int, 5>{});
    static_assert(squares.size() == 5);
    static_assert(squares[4] == 4);
    std::cout << "integer_sequence 展开：" << squares[0] << ' ' << squares[1]
              << ' ' << squares[2] << ' ' << squares[3] << ' ' << squares[4] << '\n';
}
