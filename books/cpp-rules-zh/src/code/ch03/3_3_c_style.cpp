// 第 3.3 节：类型不完整时，C 风格转换会悄悄退化成按位重解释
#include <cstdio>

struct Base1 {
    virtual ~Base1() = default;
    int a = 1;
};

struct Base2 {
    virtual ~Base2() = default;
    int b = 2;
};

struct Derived;

// 此刻 Derived 只有前置声明：C 风格转换找不到继承关系，只能按位重解释
Derived* c_style(Base2* base) {
    return (Derived*)base;
}

struct Derived : Base1, Base2 {
    int value = 7;
};

int main() {
    Derived object;
    Base2* base = &object;

    // 类型完整时，static_cast 会做基类子对象到完整对象的指针调整
    Derived* correct = static_cast<Derived*>(base);
    Derived* wrong = c_style(base);

    std::printf("correct->value = %d\n", correct->value);
    std::printf("same address    = %d\n", correct == wrong);
}
