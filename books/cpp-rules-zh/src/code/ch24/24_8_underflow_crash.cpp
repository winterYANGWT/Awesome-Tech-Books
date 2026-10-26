// 第 24.8 节：size() 是无符号数，size() - n 会回绕成巨大的循环上界
// mode: crash
// expect: heap-buffer-overflow
#include <cstddef>
#include <iostream>
#include <vector>

int main() {
    std::vector<int> readings{10, 20, 30};
    const std::size_t missing = 5;

    // 3 - 5 不会得到 -2，而是无符号数的最大值减一
    std::cout << "readings.size() - missing = " << readings.size() - missing << '\n';

    for (std::size_t i = 0; i < readings.size() - missing; ++i) {
        // 用 data() 绕开 vector::operator[] 的调试断言，让 ASan 抓越界
        std::cout << readings.data()[i] << '\n';  // i 很快越过 size()
    }
}
