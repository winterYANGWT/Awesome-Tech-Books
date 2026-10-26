// 第 1.3 节：同一段代码放回 C++14，C++17 的 std::optional 就不存在
// mode: fail
// std: c++14
// expect: optional

#include <iostream>
#include <optional>

int main() {
    const std::optional<int> maybe = 42;
    std::cout << *maybe << '\n';
}
