import catalog;

#include <iostream>

int main() {
    // 接口里只有声明，实现单元只显式实例化了 int 和 double。
    std::cout << store::discounted(1000LL) << '\n';
}
