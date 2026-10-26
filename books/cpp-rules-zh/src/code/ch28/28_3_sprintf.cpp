// 第 28.3 节：sprintf 不知道目标缓冲区多大，长度越界在检测之前已经发生
// mode: crash
// expect: stack-buffer-overflow
#include <cstdio>
#include <iostream>

int main() {
    char text[8];
    // 通过 volatile 和 argc 阻止编译器把值折叠成常量，模拟运行期才拿到的订单号。
    volatile int digits = 100000000 + 1;
    const int orderId = digits;
    std::sprintf(text, "%d", orderId);
    std::cout << "写完了: " << text << '\n';
}
