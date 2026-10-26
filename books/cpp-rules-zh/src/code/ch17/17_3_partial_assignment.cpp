// 第 17.3 节：经具体基类指针赋值只会复制基类部分
#include <iostream>
#include <string>

class Animal {
public:
    Animal(std::string name, int legs) : name_(std::move(name)), legs_(legs) {}
    virtual ~Animal() = default;

    virtual std::string sound() const { return "..."; }
    Animal& operator=(const Animal&) = default;

    const std::string& name() const { return name_; }
    int legs() const { return legs_; }

private:
    std::string name_;
    int legs_;
};

class Lizard : public Animal {
public:
    Lizard(std::string name, bool hasTail)
        : Animal(std::move(name), 4), hasTail_(hasTail) {}

    std::string sound() const override { return "hiss"; }
    bool hasTail() const { return hasTail_; }

private:
    bool hasTail_;
};

int main() {
    Lizard liz("Liz", true);
    const Lizard other("Other", false);

    std::cout << "赋值前：" << liz.name() << "，" << liz.legs() << " 条腿，"
              << (liz.hasTail() ? "有尾巴" : "没尾巴") << '\n';

    Animal* base = &liz;
    *base = other;  // 只调用 Animal::operator=，Lizard 部分没有参与

    std::cout << "赋值后：" << liz.name() << "，" << liz.legs() << " 条腿，"
              << (liz.hasTail() ? "有尾巴" : "没尾巴") << '\n';
    std::cout << "other 本来" << (other.hasTail() ? "有尾巴" : "没尾巴")
              << "，liz 的尾巴状态却没有跟着变。\n";
}
