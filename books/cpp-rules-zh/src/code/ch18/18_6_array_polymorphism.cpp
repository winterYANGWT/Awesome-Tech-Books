// 第 18.6 节：数组按静态元素类型算步长，派生数组不能当基类数组用
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class Base {
public:
    virtual ~Base() = default;
    virtual std::string label() const { return "Base"; }
    int baseValue() const { return baseValue_; }

private:
    int baseValue_ = 1;
};

class Derived : public Base {
public:
    std::string label() const override { return "Derived"; }
    int derivedValue() const { return derivedValue_; }
    int extra() const { return extra_; }

private:
    int derivedValue_ = 2;
    int extra_ = 3;
};

int main() {
    std::cout << "sizeof(Base)=" << sizeof(Base)
              << "，sizeof(Derived)=" << sizeof(Derived) << '\n';

    // 正确做法：容器里放指向基类的指针，每个元素单独分配。
    std::vector<std::unique_ptr<Base>> items;
    items.push_back(std::make_unique<Derived>());
    items.push_back(std::make_unique<Derived>());
    std::cout << "指针容器：";
    for (const auto& item : items) {
        std::cout << ' ' << item->label();
    }
    std::cout << '\n';

    // 这里只声明，不通过 base 做下标运算；下一节的 crash 示例会故意越界。
    Derived array[2];
    Base* base = array;
    std::cout << "第一个元素经基类指针调用：" << base[0].label()
              << "（baseValue=" << base[0].baseValue()
              << "，derivedValue=" << array[0].derivedValue()
              << "，extra=" << array[0].extra() << "）\n";
    std::cout << "但 base[1] 的地址不是 array[1]，步长是 sizeof(Base)=" << sizeof(Base)
              << " 而不是 sizeof(Derived)=" << sizeof(Derived) << '\n';

    static_assert(sizeof(Derived) > sizeof(Base));
    static_assert(std::is_polymorphic_v<Base>);
}
