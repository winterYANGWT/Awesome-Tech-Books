// 第 17.5 节：少写 const 会变成隐藏；Base::f() 会关掉虚分发
// flags: -Wno-overloaded-virtual
#include <iostream>
#include <string>

class Base {
public:
    virtual ~Base() = default;
    virtual std::string name() const { return "Base"; }
    virtual std::string describe() const { return "Base::describe"; }
    virtual void log() const { std::cout << "Base::log\n"; }
};

class Derived : public Base {
public:
    // 少了 const：签名不匹配，这不是覆盖，而是一个新的非 const 成员函数。
    // clang 的 -Woverloaded-virtual 会警告；示例关掉警告以便展示运行结果。
    std::string describe() { return "Derived::describe"; }

    std::string name() const override { return "Derived"; }

    // 限定调用 Base::name()：明确要基类实现，不参与动态分派。
    std::string extendedName() const { return Base::name() + "+" + name(); }

    // 覆盖里复用基类实现：不写 Base::log() 就会递归调用自己。
    void log() const override {
        Base::log();
        std::cout << "Derived::log\n";
    }
};

int main() {
    Derived derived;
    const Base& constBase = derived;
    Base& base = derived;

    std::cout << "derived.describe(): " << derived.describe() << '\n';
    std::cout << "constBase.describe(): " << constBase.describe() << '\n';
    std::cout << "base.describe(): " << base.describe() << '\n';

    std::cout << "derived.name(): " << derived.name() << '\n';
    std::cout << "base.name(): " << base.name() << '\n';
    std::cout << "base.Base::name(): " << base.Base::name() << '\n';
    std::cout << "extendedName(): " << derived.extendedName() << '\n';

    derived.log();
    base.log();
}
