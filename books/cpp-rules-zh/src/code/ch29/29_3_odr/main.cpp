// 第 29.3 节：在多个翻译单元之间验证 inline 函数和 inline 变量只有一份
#include "counter.h"

#include <iostream>

int main() {
    std::cout << std::boolalpha;
    std::cout << "a_bump=" << a_bump() << " b_bump=" << b_bump() << '\n';
    std::cout << "calls()=" << calls() << '\n';
    std::cout << "a/b calls 地址相同: " << (a_calls_address() == b_calls_address()) << '\n';
    std::cout << "a/b shared_value 地址相同: "
              << (a_shared_address() == b_shared_address()) << '\n';
    std::cout << "a/b square 地址相同: "
              << (a_square_address() == b_square_address()) << '\n';
    std::cout << "shared_value=" << shared_value
              << " twice(21)=" << twice(21) << '\n';
}
