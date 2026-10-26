// 第 36.4 节：断言表达式带副作用时，调试构建和发布构建的行为不同
#include <cassert>
#include <iostream>

int checks = 0;

bool positive(int value) {
    ++checks;
    return value > 0;
}

int main() {
    int value = 0;
    assert(positive(++value));  // 两次副作用都藏在 assert 里
    std::cout << "value=" << value << " checks=" << checks << '\n';
}
