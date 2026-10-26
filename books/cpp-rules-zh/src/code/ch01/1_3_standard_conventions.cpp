// 第 1.3 节：标准库按约定工作，不靠继承；标准版本和特性宏决定你能用什么

#include <array>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <string>
#include <type_traits>
#include <vector>
#include <version>

// STL 的算法只要求迭代器支持 !=、*、++ 和复制，不要求任何基类。
template <class It, class T>
It find_value(It first, It last, const T& value) {
    while (first != last && *first != value) {
        ++first;
    }
    return first;
}

int main() {
    // 标准库里的 string 不是独立的类，而是 basic_string<char> 的别名。
    static_assert(std::is_same_v<std::string, std::basic_string<char>>);

    // 这两个宏由实现提供：__cplusplus 是语言版本，__cpp_concepts 是特性测试宏。
    static_assert(__cplusplus >= 202002L);
    static_assert(__cpp_concepts >= 201907L);

    int raw[]{10, 20, 30, 40};
    const auto* raw_it = find_value(std::begin(raw), std::end(raw), 30);
    std::cout << "raw index: " << (raw_it - std::begin(raw)) << '\n';

    const std::vector<int> values{10, 20, 30, 40};
    const auto vec_it = find_value(values.begin(), values.end(), 30);
    std::cout << "vector index: " << std::distance(values.begin(), vec_it) << '\n';

    const auto missing = find_value(values.begin(), values.end(), 99);
    std::cout << "missing at end: " << (missing == values.end()) << '\n';

    std::cout << "__cplusplus=" << __cplusplus << '\n';
    std::cout << "__cpp_concepts=" << __cpp_concepts << '\n';
#ifdef __cpp_lib_ranges
    std::cout << "__cpp_lib_ranges=" << __cpp_lib_ranges << '\n';
#else
    std::cout << "__cpp_lib_ranges=not defined\n";
#endif
}
