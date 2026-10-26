// 第 19.4 节：unique_ptr 不支持协变返回，clone 在派生类里不能改返回类型
// mode: fail
// expect: covariant
#include <memory>

struct Base {
    virtual ~Base() = default;
    virtual std::unique_ptr<Base> clone() const = 0;
};

struct Derived : Base {
    std::unique_ptr<Derived> clone() const override {
        return std::make_unique<Derived>(*this);
    }
};

int main() {
    Derived derived;
    auto copy = derived.clone();
}
