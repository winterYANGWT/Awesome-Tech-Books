// 第 17.7 节：clone 的协变返回类型，以及经基类指针调用时的静态类型
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

class Shape {
public:
    virtual ~Shape() = default;
    [[nodiscard]] virtual Shape* clone() const = 0;
    virtual std::string name() const = 0;
};

class Circle : public Shape {
public:
    // 协变返回：基类返回 Shape*，这里可以返回更具体的 Circle*。
    [[nodiscard]] Circle* clone() const override { return new Circle(*this); }
    std::string name() const override { return "Circle"; }
};

class Square : public Shape {
public:
    [[nodiscard]] Square* clone() const override { return new Square(*this); }
    std::string name() const override { return "Square"; }
};

int main() {
    std::unique_ptr<Shape> base = std::make_unique<Circle>();

    // 经 Shape* 调用，静态类型仍是 Shape*，但动态类型被复制。
    std::unique_ptr<Shape> copy(base->clone());
    std::cout << "通过 Shape* clone：" << copy->name() << '\n';

    // 直接通过 Circle 调用，返回类型就是 Circle*，不需要向下转型。
    const Circle circle;
    std::unique_ptr<Circle> concrete(circle.clone());
    std::cout << "直接通过 Circle clone：" << concrete->name() << '\n';

    static_assert(std::is_same_v<decltype(std::declval<const Circle&>().clone()), Circle*>);
    static_assert(std::is_same_v<decltype(std::declval<const Shape&>().clone()), Shape*>);
    std::cout << "static_assert：Circle::clone 返回 Circle*，Shape::clone 返回 Shape*\n";
}
