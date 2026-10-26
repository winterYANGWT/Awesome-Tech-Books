import catalog;

#include <iostream>

int main() {
    std::cout << "int 1000: " << store::discounted(1000) << '\n';
    std::cout << "double 9.5: " << store::discounted(9.5) << '\n';
    static_assert(sizeof(store::Item) == sizeof(int));
    std::cout << "Item 大小: " << sizeof(store::Item) << '\n';
}
