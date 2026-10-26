// 第 33.4 节：别名给类型语义名，把可替换点集中到一处
#include <cstdint>
#include <deque>
#include <iostream>
#include <type_traits>
#include <vector>

namespace shop {
struct Item {
    const char* name;
    int cents;
};

using Cents = std::int64_t;          // 语义名，不是“又一个 long long”
using Cart = std::deque<Item>;       // 换容器只改这一行
using CartIterator = Cart::const_iterator;
using Discount = Cents (*)(Cents, int);

static_assert(std::is_same_v<Cart::value_type, Item>);
static_assert(std::is_same_v<CartIterator, std::deque<Item>::const_iterator>);

// Inches 和 Dollars 只是 int 的同义词，不是新的强类型
using Inches = int;
using Dollars = int;
static_assert(std::is_same_v<Inches, Dollars>);

Cents subtotal(const Cart& cart) {
    Cents total = 0;
    for (CartIterator it = cart.begin(); it != cart.end(); ++it) {
        total += it->cents;
    }
    return total;
}

Cents ten_percent_off(Cents cents, int percent) {
    return cents - cents * percent / 100;
}

template<class Sequence>
Cents subtotal_generic(const Sequence& items) {
    Cents total = 0;
    for (const auto& item : items) total += item.cents;
    return total;
}
}

int main() {
    shop::Cart cart{{{"pen", 250}, {"notebook", 600}}};
    std::vector<shop::Item> vector{{{"pen", 250}, {"notebook", 600}}};
    shop::Discount discount = shop::ten_percent_off;

    std::cout << "cart=" << shop::subtotal(cart) << '\n';
    std::cout << "vector=" << shop::subtotal_generic(vector) << '\n';
    std::cout << "discount=" << discount(shop::subtotal(cart), 10) << '\n';
    std::cout << "Inches and Dollars are the same type: "
              << (std::is_same_v<shop::Inches, shop::Dollars> ? "yes" : "no") << '\n';
}
