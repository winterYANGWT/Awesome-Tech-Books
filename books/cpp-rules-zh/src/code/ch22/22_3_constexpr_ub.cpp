// 第 22.3 节：constexpr 函数带运行期实参调用时，未定义行为不会被编译期检查拦住
// mode: crash
// expect: runtime error: division by zero
#include <iostream>

constexpr int ratio(int total, int parts) { return total / parts; }

int main() {
    volatile int parts = 0;  // 运行期才知道是 0
    std::cout << ratio(10, parts) << '\n';
}
