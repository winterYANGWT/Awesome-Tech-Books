// 第 30.4 节：C++20 起对 volatile 对象使用 ++/-- 已被弃用
// mode: warn
// expect: -Wdeprecated-volatile
#include <iostream>

volatile int ticks = 0;

int main() {
    ++ticks;                     // 单线程下没有竞争，只是展示弃用警告
    std::cout << ticks << '\n';
}
