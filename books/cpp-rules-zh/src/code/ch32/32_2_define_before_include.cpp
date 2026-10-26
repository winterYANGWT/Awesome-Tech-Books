// 第 32.2 节：在标准头文件之前定义 min 宏，会把库里的声明改坏
// mode: fail
// expect: min
#include <iostream>

#define min(a, b) ((a) < (b) ? (a) : (b))
#include <algorithm>

int main() {
    std::cout << std::min(3, 5) << '\n';
}
