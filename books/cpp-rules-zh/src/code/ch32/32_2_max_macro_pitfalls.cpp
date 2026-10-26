// 第 32.2 节：max 宏的括号、重复求值和整体展开陷阱
#include <algorithm>
#include <iostream>

// 参数和整体都没加括号：优先级一变，语义就变了。
#define BAD_MAX(a, b) a < b ? b : a
// 参数和整体都加了括号，但实参仍然出现两次。
#define PAREN_MAX(a, b) ((a) < (b) ? (b) : (a))
// 参数不加括号：2 + 2 先做除法。
#define HALF(x) x / 2

int measured_calls = 0;
int measured() {
    ++measured_calls;
    return 7;
}

int main() {
    const int i = 1, j = 5;

    // 展开成 i < j ? j : i + 42，+42 只落在其中一个分支上。
    const int bad = BAD_MAX(i, j) + 42;
    std::cout << "BAD_MAX(i, j) + 42 = " << bad << '\n';
    // 整体加括号后，+42 落在条件表达式外面。
    const int good = PAREN_MAX(i, j) + 42;
    std::cout << "PAREN_MAX(i, j) + 42 = " << good << '\n';
    // 展开成 2 + 2 / 2。
    std::cout << "HALF(2 + 2) = " << HALF(2 + 2) << '\n';

    // measured() 在条件里调用一次，在选中的分支里又调用一次。
    measured_calls = 0;
    const int low = PAREN_MAX(measured(), 0);
    std::cout << "PAREN_MAX(measured(), 0) = " << low
              << "，调用 " << measured_calls << " 次\n";

    // 这次条件为真，选中的是另一个实参，measured() 只调用一次。
    measured_calls = 0;
    const int high = PAREN_MAX(measured(), 100);
    std::cout << "PAREN_MAX(measured(), 100) = " << high
              << "，调用 " << measured_calls << " 次\n";

    // std::max 的实参各求值一次，和比较结果无关。
    measured_calls = 0;
    const int from_std = std::max(measured(), 0);
    std::cout << "std::max(measured(), 0) = " << from_std
              << "，调用 " << measured_calls << " 次\n";
}
