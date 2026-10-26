// 第 11.3 节：构造期间调用没有定义的纯虚函数，程序终止
// mode: crash
// expect: pure virtual
#include <iostream>

struct Base {
    Base() {
        std::cout << "Base 构造函数调用 call_init()\n";
        call_init(*this);                 // 经普通函数绕一下，两个编译器都不好直接判断
    }
    virtual void init() = 0;
    virtual ~Base() = default;

    static void call_init(Base& base) { base.init(); }
};

struct Derived : Base {
    void init() override { std::cout << "Derived::init\n"; }
};

int main() {
    Derived object;
    std::cout << "构造完成\n";
}
