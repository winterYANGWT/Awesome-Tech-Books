// 第 22.3 节：consteval 函数不能用运行期实参调用
// mode: fail
// expect: not a constant expression
#include <iostream>

consteval int twice(int x) { return x * 2; }

int main() {
    int n = 21;
    std::cout << twice(n) << '\n';
}
