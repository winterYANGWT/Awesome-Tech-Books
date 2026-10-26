// 第 17.7 节：unique_ptr 不是协变返回类型
// mode: fail
// expect: different return type
#include <memory>

class Shape {
public:
    virtual ~Shape() = default;
    virtual std::unique_ptr<Shape> clone() const = 0;
};

class Circle : public Shape {
public:
    // unique_ptr<Circle> 并不是 unique_ptr<Shape> 的派生类，不能协变。
    std::unique_ptr<Circle> clone() const override {
        return std::make_unique<Circle>(*this);
    }
};

int main() {
    Circle circle;
    auto copy = circle.clone();
    (void)copy;
}
