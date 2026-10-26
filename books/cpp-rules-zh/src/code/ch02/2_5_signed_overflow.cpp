// 第 2.5 节：反例：有符号整数溢出仍是未定义行为
// mode: crash
// expect: signed integer overflow
#include <iostream>
#include <limits>

int main() {
    int total = std::numeric_limits<int>::max();
    std::cout << "before: " << total << '\n';

    // 不是“回绕成负数”，而是未定义行为；UBSan 会在这里停下。
    total += 1;

    std::cout << "after: " << total << '\n';
}
