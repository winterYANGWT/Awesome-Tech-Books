import shop;

#include <iostream>

int main() {
    const double net = shop::subtotal(12.5, 4);
    std::cout << "小计: " << net << '\n';
    std::cout << "含税: " << shop::with_tax(net) << '\n';
    std::cout << "包装费: " << shop::packaging_fee(net) << '\n';
}
