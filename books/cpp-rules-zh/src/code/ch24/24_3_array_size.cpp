// 第 24.3 节：长度交给 std::size 和容器，别依赖只在数组作用域里成立的 sizeof 除法
#include <array>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// 想保留数组类型，就用数组引用作形参，N 可以在编译期拿到
template <class T, std::size_t N>
constexpr std::size_t array_length(const T (&)[N]) {
    return N;
}

int main() {
    int grades[8]{90, 85, 77, 92, 60, 88, 95, 71};

    // 在当前作用域里，grades 还是数组，这个除法碰巧是对的
    std::cout << "作用域内 sizeof 除法: " << sizeof(grades) / sizeof(*grades) << '\n';
    std::cout << "std::size(数组): " << std::size(grades) << '\n';
    std::cout << "std::ssize(数组): " << std::ssize(grades) << '\n';
    std::cout << "数组引用模板: " << array_length(grades) << '\n';
    static_assert(array_length(grades) == 8);

    // 固定长度用 std::array，长度仍然跟着类型走
    std::array<int, 8> fixed{90, 85, 77, 92, 60, 88, 95, 71};
    std::cout << "std::array 的 size(): " << fixed.size()
              << "，std::size: " << std::size(fixed) << '\n';

    // 长度不固定就用 vector/string；它们自己管理内存和长度
    std::vector<int> dynamic(grades, grades + std::size(grades));
    dynamic.push_back(100);
    std::cout << "vector 的 size(): " << dynamic.size()
              << "，capacity(): " << dynamic.capacity() << '\n';

    std::string report = "grades:";
    for (int grade : grades) {
        report += ' ';
        report += std::to_string(grade);
    }
    std::cout << report << '\n';
    std::cout << "string 的 size(): " << report.size() << '\n';

    // 对容器对象用 sizeof 除法得到的是“对象大小 / 元素大小”，不是元素个数
    std::cout << "对 vector 对象做 sizeof 除法: "
              << sizeof(dynamic) / sizeof(dynamic[0]) << "（不是元素个数）\n";
}
