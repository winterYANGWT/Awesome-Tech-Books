// 第 17.1 节：public 继承要求派生对象能顶替基类对象
#include <iostream>
#include <type_traits>

class Rectangle {
public:
    Rectangle(int width, int height) : width_(width), height_(height) {}
    virtual ~Rectangle() = default;

    virtual void setWidth(int width) { width_ = width; }
    virtual void setHeight(int height) { height_ = height; }
    int width() const { return width_; }
    int height() const { return height_; }

private:
    int width_;
    int height_;
};

// 正方形“是”矩形吗？数学上是，行为上不是：矩形允许宽高独立变化。
class Square : public Rectangle {
public:
    explicit Square(int side) : Rectangle(side, side) {}

    // 为了维持正方形的自有不变式，改宽必须同时改高。
    void setWidth(int width) override {
        Rectangle::setWidth(width);
        Rectangle::setHeight(width);
    }
    void setHeight(int height) override {
        Rectangle::setHeight(height);
        Rectangle::setWidth(height);
    }
};

// 这个函数只依赖 Rectangle 的公开契约：改宽不动高。
void growWidth(Rectangle& rect, int delta) {
    const int oldHeight = rect.height();
    rect.setWidth(rect.width() + delta);
    std::cout << "  宽=" << rect.width() << " 高=" << rect.height();
    if (rect.height() != oldHeight) {
        std::cout << "  <- 高度被改了，Rectangle 的契约不再成立";
    }
    std::cout << '\n';
}

int main() {
    Rectangle rect(10, 5);
    std::cout << "普通矩形，加宽 5：\n";
    growWidth(rect, 5);

    Square square(10);
    std::cout << "正方形，加宽 5：\n";
    growWidth(square, 5);

    // 类型系统只检查“是不是派生自”，检查不了行为契约。
    static_assert(std::is_base_of_v<Rectangle, Square>);
    std::cout << "编译器接受了 Square is-a Rectangle；上面却已经破坏了矩形的契约。\n";
}
