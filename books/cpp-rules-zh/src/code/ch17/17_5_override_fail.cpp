// 第 17.5 节：签名不匹配时写 override 会把静默隐藏变成编译错误
// mode: fail
// expect: override
#include <string>

class Base {
public:
    virtual ~Base() = default;
    virtual std::string describe() const { return "Base"; }
};

class Derived : public Base {
public:
    // 少了 const，override 立刻指出它没有覆盖任何东西。
    std::string describe() override { return "Derived"; }
};

int main() {
    Derived d;
    return d.describe().empty() ? 1 : 0;
}
