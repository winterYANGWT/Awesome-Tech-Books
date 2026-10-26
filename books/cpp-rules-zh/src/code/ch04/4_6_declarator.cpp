// 第 4.6 节：int* a, b, c; 里只有 a 是指针
#include <iostream>
#include <type_traits>

int main() {
    int x = 1;
    int y = 2;

    int* a, b, c;   // * 只绑定紧跟的 a；b、c 是普通 int
    static_assert(std::is_same_v<decltype(a), int*>);
    static_assert(std::is_same_v<decltype(b), int>);
    static_assert(std::is_same_v<decltype(c), int>);

    a = &x;
    b = y;
    c = 3;
    std::cout << "*a = " << *a << "，b = " << b << "，c = " << c << "\n";

    // 引用有同样的陷阱：只有 r 是引用，s 是普通 int
    int& r = x, s = y;
    static_assert(std::is_same_v<decltype(r), int&>);
    static_assert(std::is_same_v<decltype(s), int>);

    r = 10;   // 改的是 x
    s = 20;   // 改的是 s，y 不变
    std::cout << "x = " << x << "，y = " << y << "，s = " << s << "\n";
}
