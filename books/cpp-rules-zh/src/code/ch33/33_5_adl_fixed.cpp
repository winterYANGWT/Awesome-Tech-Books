// 第 33.5 节：把运算符放回类型命名空间；无关的模板运算符也别混进去
#include <cstddef>
#include <iostream>
#include <numeric>

namespace catalog {
struct Item {
    int cents;
};

// 和 Item 放在一起，ADL 在 int + Item 时能找到它。
int operator+(int total, const Item& item) { return total + item.cents; }
}

namespace math {
// 和 shop::Money 无关的“万能”运算符，放在 math 里不会污染 shop。
template<class T>
int operator+(T, std::size_t) { return 42; }
}

namespace shop {
struct Money {
    int cents;
};
}

namespace lib {
template<class T>
struct Iter {
    T* p;
    T& operator*() const { return *p; }
};

template<class T>
Iter<T> operator+(Iter<T> it, std::ptrdiff_t n) {
    it.p += n;
    return it;
}

template<class T>
struct Box {
    T* data;
    Iter<T> begin() const { return {data}; }
    T& operator[](std::size_t i) const { return *((begin()) + i); }
};
}

int main() {
    catalog::Item items[3]{{100}, {200}, {300}};
    std::cout << "accumulate=" << std::accumulate(items, items + 3, 0) << '\n';

    shop::Money wallet[3]{{10}, {20}, {30}};
    lib::Box<shop::Money> box{wallet};
    std::cout << "box[1]=" << box[1].cents << '\n';
}
