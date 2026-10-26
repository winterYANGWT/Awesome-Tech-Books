// 第 14.3 节：private 限制名字的使用，不限制名字的可见性，也可以被合法转交
#include <iostream>

// 私有虚函数：派生类可以覆盖，但不能调用它的名字。
class Task {
public:
    void run() const { step(); }        // 非虚接口

private:
    virtual void step() const = 0;
};

class Greeting final : public Task {
private:
    void step() const override { std::cout << "hello from derived\n"; }
};

// 有访问权的成员可以把指向私有函数的成员指针交给外部。
class Meter {
public:
    using Scale = int (Meter::*)(int) const;
    Scale expose() const { return &Meter::scale; }
    [[nodiscard]] int value() const { return scale(2); }

private:
    int scale(int k) const { return base_ * k; }
    int base_ = 21;
};

// 成员模板可以被任何类型显式特化；特化体算成员，因此能碰 private。
class Vault {
public:
    template <class T>
    void audit(const T&) {
        std::cout << "audit: " << secret_ << "\n";
    }
    [[nodiscard]] int secret() const { return secret_; }

private:
    int secret_ = 7;
};

namespace {
struct Probe {};   // 独有类型，避免和别人的特化冲突
}

template <>
void Vault::audit<Probe>(const Probe&) {
    std::cout << "specialized audit\n";
    secret_ = 42;   // 合法，但工程代码里不应该出现
}

int main() {
    Greeting g;
    g.run();

    Meter m;
    auto p = m.expose();
    std::cout << "via member pointer: " << (m.*p)(2) << "\n";

    Vault v;
    v.audit(1);
    v.audit(Probe{});
    std::cout << "secret=" << v.secret() << "\n";
}
