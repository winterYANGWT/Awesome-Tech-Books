// 第 29.3 节：inline 不改变调用语义：可以取地址，可以是递归或虚函数
#include <iostream>

inline int square(int x) { return x * x; }

// inline 只表示“允许多个翻译单元重复定义”，不限制函数体长什么样。
inline int factorial(int n) { return n <= 1 ? 1 : n * factorial(n - 1); }

struct Base {
    virtual ~Base() = default;
    virtual int value() const { return 0; }
};

struct Derived : Base {
    int value() const override;   // 在类外定义，仍然可以是 inline
};

inline int Derived::value() const { return 42; }

// inline 函数里的局部 static 在所有调用点、所有翻译单元里是同一个对象。
inline int& calls() {
    static int count = 0;
    return count;
}

inline int bump() { return ++calls(); }

int main() {
    int (*fp)(int) = &square;     // 取地址：编译器必须为它生成一个真正的函数体

    std::cout << std::boolalpha;
    std::cout << "square(7)=" << square(7) << " fp(7)=" << fp(7) << '\n';
    std::cout << "&square == fp: " << (&square == fp) << '\n';
    std::cout << "factorial(5)=" << factorial(5) << '\n';

    Derived d;
    Base& b = d;
    std::cout << "virtual inline: " << b.value() << '\n';

    bump();
    bump();
    std::cout << "inline 函数内的 static: " << calls() << '\n';
    std::cout << "再次取同一个 static 的地址: " << (&calls() == &calls()) << '\n';
}
