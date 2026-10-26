#include "quote.h"

#include <iostream>

int main() {
    std::cout << "3kg 基础运费: " << shipping::base_rate(3.0) << '\n';
    std::cout << "120 分 clamp 后: " << shipping::clamp_charge(120) << '\n';
    std::cout << "80 分 clamp 后: " << shipping::clamp_charge(80) << '\n';
}
