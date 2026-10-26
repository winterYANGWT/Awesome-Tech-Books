// 第 16.7 节：移位量达到位宽是未定义行为，UBSan 会当场报错
// mode: crash
// expect: shift exponent
#include <iostream>

int main() {
    int value = 1;
    volatile int bits = 32;          // 避免编译器把越界移位直接折叠成常量
    std::cout << "shift by " << bits << '\n';
    std::cout << (value << bits) << '\n';
}
