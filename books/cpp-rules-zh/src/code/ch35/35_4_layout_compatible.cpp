// 第 35.4 节：C++20 只能比较两个 C++ 类型的布局，问不了 C 编译器
#include <cstddef>
#include <iostream>
#include <type_traits>

struct c_reading {
    int id;
    double celsius;
};

struct cpp_reading {
    int id;
    double celsius;
};

static_assert(std::is_standard_layout_v<cpp_reading>);
static_assert(std::is_layout_compatible_v<c_reading, cpp_reading>);

int main() {
    std::cout << "size=" << sizeof(cpp_reading)
              << " id@" << offsetof(cpp_reading, id)
              << " celsius@" << offsetof(cpp_reading, celsius) << '\n';
}
