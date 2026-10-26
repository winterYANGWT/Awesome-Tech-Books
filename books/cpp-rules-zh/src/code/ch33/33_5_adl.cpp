// 第 33.5 节：类型的非成员接口和类型放在同一命名空间，ADL 自然找得到
#include <iostream>
#include <string>

namespace shop {
struct Price {
    long long cents;
};

Price operator+(Price lhs, Price rhs) {
    return Price{lhs.cents + rhs.cents};
}

std::string label(const Price& price) {
    return std::to_string(price.cents) + " cents";
}
}

int main() {
    shop::Price coffee{1250};
    shop::Price cake{375};
    shop::Price total = coffee + cake;   // ADL 找到 shop::operator+
    std::cout << label(total) << '\n';   // 全局没有 label，ADL 找到 shop::label
}
