// 第 16.2 节：后置 ++ 带 & 限定，s++++ 在编译期被拒绝
// mode: fail
// expect: cannot increment
#include <iostream>

class Counter {
public:
    Counter& operator++();
    Counter operator++(int) &;
};

int main() {
    Counter c;
    c++++;                  // 第二次 ++ 要作用于第一次返回的纯右值，没有可调用的成员
    std::cout << "不会编译到这里\n";
}
