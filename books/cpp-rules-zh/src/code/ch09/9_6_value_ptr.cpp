// 第 9.6 节：类成员的智能指针要提供值语义：深拷贝、copy-and-swap、按引用解引用
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

class Shape {
public:
    Shape() { ++live; }
    Shape(const Shape&) { ++live; }
    virtual ~Shape() { --live; }

    virtual Shape* clone() const = 0;
    virtual double area() const = 0;
    virtual void scale(double factor) = 0;

    static int live;
};
int Shape::live = 0;

class Circle : public Shape {
public:
    explicit Circle(double radius) : radius_(radius) {}
    Shape* clone() const override { return new Circle(*this); }
    double area() const override { return 3.141592653589793 * radius_ * radius_; }
    void scale(double factor) override { radius_ *= factor; }

private:
    double radius_;
};

class Square : public Shape {
public:
    explicit Square(double side) : side_(side) {}
    Shape* clone() const override { return new Square(*this); }
    double area() const override { return side_ * side_; }
    void scale(double factor) override { side_ *= factor; }

private:
    double side_;
};

// 克隆策略集中在一个可以特化的 traits 类里。
template <typename T>
struct CloneTraits {
    static T* clone(const T& value) { return new T(value); }
};

// 值语义智能指针：拷贝时深拷贝，赋值用 copy-and-swap。
template <typename T>
class ValuePtr {
public:
    explicit ValuePtr(T* p = nullptr) noexcept : p_(p) {}
    ~ValuePtr() { delete p_; }

    ValuePtr(const ValuePtr& other)
        : p_(other.p_ ? CloneTraits<T>::clone(*other.p_) : nullptr) {}

    ValuePtr(ValuePtr&& other) noexcept : p_(other.p_) { other.p_ = nullptr; }

    ValuePtr& operator=(const ValuePtr& other) {
        ValuePtr temp(other);
        swap(temp);
        return *this;
    }

    ValuePtr& operator=(ValuePtr&& other) noexcept {
        ValuePtr temp(std::move(other));
        swap(temp);
        return *this;
    }

    void swap(ValuePtr& other) noexcept { std::swap(p_, other.p_); }

    T& operator*() const { return *p_; }
    T* operator->() const { return p_; }
    explicit operator bool() const noexcept { return p_ != nullptr; }

private:
    T* p_;
};

// 多态类型不能按值 new T(*p)，交给虚 clone()。
template <>
struct CloneTraits<Shape> {
    static Shape* clone(const Shape& value) { return value.clone(); }
};

class Drawing {
public:
    explicit Drawing(ValuePtr<Shape> shape) : shape_(std::move(shape)) {}
    Drawing(const Drawing& other) : shape_(other.shape_) {}
    Drawing(Drawing&&) noexcept = default;
    ~Drawing() = default;

    Drawing& operator=(Drawing other) {
        swap(other);
        return *this;
    }

    void swap(Drawing& other) noexcept { shape_.swap(other.shape_); }

    double area() const { return shape_ ? shape_->area() : 0.0; }
    void scale(double factor) {
        if (shape_) {
            shape_->scale(factor);
        }
    }

private:
    ValuePtr<Shape> shape_;
};

struct Config {
    std::string name;
    int retries;
};

int main() {
    static_assert(std::is_same_v<decltype(*std::declval<ValuePtr<Config>&>()),
                                 Config&>);

    std::cout << "== 多态值语义 ==\n";
    {
        Drawing first(ValuePtr<Shape>(new Circle(2.0)));
        Drawing second = first;   // 深拷贝：Circle 被 clone
        Drawing third(ValuePtr<Shape>(new Square(3.0)));

        std::cout << "初始面积：first=" << first.area()
                  << "，second=" << second.area()
                  << "，third=" << third.area() << "\n";

        second.scale(2.0);        // 只改 second 自己的副本
        std::cout << "second 放大后：first=" << first.area()
                  << "，second=" << second.area() << "\n";

        third = first;            // copy-and-swap：旧的 Square 被释放
        std::cout << "third 赋值为 first 后面积：" << third.area() << "\n";

        Drawing fourth = std::move(third);
        std::cout << "移动后 fourth 面积：" << fourth.area() << "\n";
    }
    std::cout << "多态对象剩余：" << Shape::live << " 个\n";

    std::cout << "== 普通值类型 ==\n";
    {
        ValuePtr<Config> production(new Config{"prod", 3});
        ValuePtr<Config> staging = production;
        staging->retries = 5;
        std::cout << "production: " << production->name << "/"
                  << production->retries << "，staging: " << staging->name
                  << "/" << staging->retries << "\n";
    }
    std::cout << "多态对象剩余：" << Shape::live << " 个\n";
}
