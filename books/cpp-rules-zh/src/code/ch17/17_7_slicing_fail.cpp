// 第 17.7 节：抽象基类不能用按值传参，切片在编译期就被挡住
// mode: fail
// expect: abstract
#include <string>

class Shape {
public:
    virtual ~Shape() = default;
    virtual std::string name() const = 0;
    virtual int area() const = 0;
};

class Circle : public Shape {
public:
    std::string name() const override { return "Circle"; }
    int area() const override { return 3; }
};

// 按值接收 Shape 会试图创建一个 Shape 对象；Shape 是抽象类，这里直接非法。
void print(Shape shape) {
    (void)shape;
}

int main() {
    Circle circle;
    print(circle);  // 即使实参是 Circle，形参类型仍然是抽象的 Shape
}
