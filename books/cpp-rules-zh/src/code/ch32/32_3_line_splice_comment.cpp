// 第 32.3 节：C++17 去掉了 trigraph，但反斜杠续行仍能把下一行并进注释
// flags: -Wno-comment
// expect: retries=0
#include <iostream>

int main() {
    int retries = 0;

    // 这行注释以反斜杠结尾，下一行属于注释 \
    ++retries;

    std::cout << "retries=" << retries << '\n';
}
