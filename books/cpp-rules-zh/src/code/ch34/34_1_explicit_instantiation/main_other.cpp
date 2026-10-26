#include "quote.h"

#include <iostream>

int main() {
    // 头文件里只对 int 做了 extern template；double 版没有定义可实例化。
    std::cout << shipping::clamp_charge(120.0) << '\n';
}
