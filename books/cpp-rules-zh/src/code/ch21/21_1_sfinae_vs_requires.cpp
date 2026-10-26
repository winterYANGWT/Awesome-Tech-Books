// 第 21.1 节：同一个“能相加”的约束，用 void_t 和 requires 各写一遍
#include <concepts>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

namespace legacy {
// C++17 的写法：条件藏进默认模板实参，签名本身看不出要求。
template <typename T,
          typename = std::void_t<decltype(std::declval<T>() + std::declval<T>())>>
T add(const T& lhs, const T& rhs) {
    return lhs + rhs;
}
}  // namespace legacy

namespace modern {
// C++20 的写法：约束直接写在模板头部，调用方一眼能看见。
template <typename T>
    requires requires(const T& lhs, const T& rhs) { lhs + rhs; }
T add(const T& lhs, const T& rhs) {
    return lhs + rhs;
}
}  // namespace modern

int main() {
    std::cout << "旧写法：\n";
    std::cout << "  " << legacy::add(40, 2) << '\n';
    std::cout << "  " << legacy::add(1.5, 3.0) << '\n';
    std::cout << "  " << legacy::add(std::string("in"), std::string("voice")) << '\n';

    std::cout << "新写法：\n";
    std::cout << "  " << modern::add(40, 2) << '\n';
    std::cout << "  " << modern::add(1.5, 3.0) << '\n';
    std::cout << "  " << modern::add(std::string("in"), std::string("voice")) << '\n';

    // 两个版本接受的类型集合相同：区别在于要求写在哪里、报错怎么说。
    static_assert(requires { legacy::add(1, 2); });
    static_assert(requires { modern::add(1, 2); });
    static_assert(requires { modern::add(std::string("a"), std::string("b")); });
}
