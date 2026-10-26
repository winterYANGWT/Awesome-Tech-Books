// 第 19.3 节：visitor 漏掉一个 variant 备选类型时，std::visit 拒绝编译
// mode: fail
// expect: invoke_result
#include <variant>

struct Circle {
    double radius;
};

struct Rect {
    double width;
    double height;
};

struct Triangle {
    double base;
    double height;
};

using Shape = std::variant<Circle, Rect, Triangle>;

struct Draw {
    void operator()(const Circle&) const {}
    void operator()(const Rect&) const {}
    // 故意不写 Triangle 的重载。
};

int main() {
    Shape shape = Triangle{3.0, 4.0};
    std::visit(Draw{}, shape);
}
