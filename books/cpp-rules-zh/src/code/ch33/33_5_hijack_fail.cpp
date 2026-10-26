// 第 33.5 节：无关的模板运算符进了类型命名空间，会劫持泛型容器的 operator[]
// mode: fail
// expect: int
#include <cstddef>

namespace shop {
struct Money {
    int cents;
};

// 和 Money 的接口毫无关系，却和 Money 同处 shop。
template<class T>
int operator+(T, std::size_t) { return 42; }
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
    shop::Money wallet[3]{{10}, {20}, {30}};
    lib::Box<shop::Money> box{wallet};
    return box[1].cents;
}
