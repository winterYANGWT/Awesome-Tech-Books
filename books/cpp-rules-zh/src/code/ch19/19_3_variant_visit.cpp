// 第 19.3 节：封闭类型集合用 std::variant + std::visit，新增分支会被编译期检查
#include <iostream>
#include <string>
#include <variant>
#include <vector>

struct Circle {
    double radius;
    std::string name() const { return "circle"; }
    double area() const { return 3.141592653589793 * radius * radius; }
};

struct Rect {
    double width;
    double height;
    std::string name() const { return "rect"; }
    double area() const { return width * height; }
};

struct Triangle {
    double base;
    double height;
    std::string name() const { return "triangle"; }
    double area() const { return 0.5 * base * height; }
};

using Shape = std::variant<Circle, Rect, Triangle>;
static_assert(std::variant_size_v<Shape> == 3);

void show(const Shape& shape) {
    // 泛型 lambda 对每个备选类型各实例化一次；少一个 name() 或 area() 都会编译失败。
    std::visit([](const auto& value) {
        std::cout << value.name() << ": area=" << value.area() << '\n';
    }, shape);
}

int main() {
    std::vector<Shape> shapes{Circle{1.0}, Rect{2.0, 3.0}, Triangle{4.0, 5.0}};
    for (const auto& shape : shapes) {
        show(shape);
    }

    std::cout << "Rect 的下标: " << Shape{Rect{1.0, 1.0}}.index() << '\n';
    std::cout << "第一个是 Circle: " << std::holds_alternative<Circle>(shapes[0]) << '\n';

    const auto& rect = std::get<Rect>(shapes[1]);
    std::cout << "取出的矩形宽: " << rect.width << '\n';
}
