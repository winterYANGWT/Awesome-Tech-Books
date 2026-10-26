// 第 22.3 节：const 只要求只读，constexpr 要求初值在编译期已知
// mode: fail
// expect: must be initialized by a constant expression
#include <iostream>

int read_limit() { return 42; }

int main() {
    int runtime = read_limit();
    const int a = runtime;       // 合法：const 变量可以用运行期值初始化
    constexpr int b = runtime;   // 错误：runtime 不是常量表达式
    std::cout << a << ' ' << b << '\n';
}
