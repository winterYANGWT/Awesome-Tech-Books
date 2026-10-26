// 第 32.3 节：C++14 的 trigraph 会把注释行尾的 ??/ 变成续行符
// std: c++14
// flags: -Wno-comment -Wno-trigraphs
// expect: attempts=0
#include <iostream>

int main() {
    int attempts = 0;
    for (int packet = 0; packet < 3; ++packet);

    // 下一行本应把 attempts 加一，注意行尾的斜杠???????????/
    ++attempts;

    std::cout << "attempts=" << attempts << '\n';
}
