// 第 19.2 节：静态多态用模板，动态多态用虚函数，std::function 把两者接起来
#include <concepts>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

struct Circle {
    double radius;
    double area() const { return 3.141592653589793 * radius * radius; }
};

struct Rect {
    double width;
    double height;
    double area() const { return width * height; }
};

// 静态多态：类型不需要有共同基类，只要满足 area() 这个隐式接口。
template <class T>
concept HasArea = requires(const T& value) {
    { value.area() } -> std::convertible_to<double>;
};

template <HasArea T>
double total_area(const std::vector<T>& items) {
    double sum = 0;
    for (const auto& item : items) {
        sum += item.area();
    }
    return sum;
}

// 动态多态：调用方只看见 Shape，具体类型可以晚于调用方编译。
class Shape {
public:
    virtual ~Shape() = default;
    virtual double area() const = 0;
    virtual std::string name() const = 0;
};

class ShapeCircle : public Shape {
public:
    explicit ShapeCircle(double radius) : radius_(radius) {}
    double area() const override { return 3.141592653589793 * radius_ * radius_; }
    std::string name() const override { return "circle"; }

private:
    double radius_;
};

class ShapeRect : public Shape {
public:
    ShapeRect(double width, double height) : width_(width), height_(height) {}
    double area() const override { return width_ * height_; }
    std::string name() const override { return "rect"; }

private:
    double width_;
    double height_;
};

double total_area(const std::vector<std::unique_ptr<Shape>>& shapes) {
    double sum = 0;
    for (const auto& shape : shapes) {
        sum += shape->area();
    }
    return sum;
}

int main() {
    static_assert(HasArea<Circle>);
    static_assert(HasArea<Rect>);
    static_assert(!HasArea<int>);

    std::cout << "静态 Circle: " << total_area(std::vector<Circle>{{1.0}}) << '\n';
    std::cout << "静态 Rect: "
              << total_area(std::vector<Rect>{{2.0, 3.0}}) << '\n';

    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<ShapeCircle>(1.0));
    shapes.push_back(std::make_unique<ShapeRect>(2.0, 3.0));
    std::cout << "动态 shapes: " << total_area(shapes) << '\n';

    // 动态帮静态：std::function 把不同具体类型擦成同一个可调用接口。
    std::vector<std::function<double()>> tasks;
    Circle circle{1.0};
    Rect rect{2.0, 3.0};
    tasks.push_back([circle] { return circle.area(); });
    tasks.push_back([rect] { return rect.area(); });
    double sum = 0;
    for (const auto& task : tasks) {
        sum += task();
    }
    std::cout << "std::function 任务: " << sum << '\n';
}
