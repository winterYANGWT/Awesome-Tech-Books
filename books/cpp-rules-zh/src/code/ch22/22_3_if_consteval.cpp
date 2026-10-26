// 第 22.3 节：C++23 的 if consteval 在函数体里区分求值环境
// std: c++23
#include <iostream>

constexpr int scale(int x) {
    if consteval {
        return x * 2;
    }
    return x * 3;
}

constexpr int compile_time = scale(7);

int main() {
    int n = 7;
    std::cout << "编译期 = " << compile_time << "，运行期 = " << scale(n) << '\n';
}
