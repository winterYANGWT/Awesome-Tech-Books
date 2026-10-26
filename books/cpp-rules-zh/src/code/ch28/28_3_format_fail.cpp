// 第 28.3 节：格式串与实参类型不匹配，std::format 在编译期就拒绝
// mode: fail
// expect: consteval
#include <format>
#include <iostream>

int main() {
    std::cout << std::format("{:d}", "not a number") << '\n';
}
