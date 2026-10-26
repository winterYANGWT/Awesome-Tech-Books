// 第 1.1 节：同一个求和任务，用 C、面向对象、模板和 STL 四种写法完成

#include <iostream>
#include <memory>
#include <numeric>
#include <vector>

namespace c_style {

struct Line {
    int unit_cents;
    int quantity;
};

int total(const Line* lines, int count) {
    int sum = 0;
    for (int i = 0; i < count; ++i) {
        int line_total = lines[i].unit_cents * lines[i].quantity;
        if (line_total >= 10000) {
            line_total = line_total * 9 / 10;
        }
        sum += line_total;
    }
    return sum;
}

}  // namespace c_style

namespace oo_style {

class Line {
public:
    Line(int unit_cents, int quantity) : unit_cents_(unit_cents), quantity_(quantity) {}
    virtual ~Line() = default;
    virtual int total_cents() const = 0;

protected:
    int unit_cents_;
    int quantity_;
};

class SimpleLine : public Line {
public:
    using Line::Line;
    int total_cents() const override { return unit_cents_ * quantity_; }
};

class BulkLine : public Line {
public:
    using Line::Line;
    int total_cents() const override {
        const int sub = unit_cents_ * quantity_;
        return sub >= 10000 ? sub * 9 / 10 : sub;
    }
};

int total(const std::vector<std::unique_ptr<Line>>& lines) {
    int sum = 0;
    for (const auto& line : lines) {
        sum += line->total_cents();
    }
    return sum;
}

}  // namespace oo_style

namespace template_style {

struct Line {
    int unit_cents;
    int quantity;
};

template <class Range>
int total(const Range& lines) {
    int sum = 0;
    for (const auto& line : lines) {
        int line_total = line.unit_cents * line.quantity;
        if (line_total >= 10000) {
            line_total = line_total * 9 / 10;
        }
        sum += line_total;
    }
    return sum;
}

}  // namespace template_style

namespace stl_style {

struct Line {
    int unit_cents;
    int quantity;
};

int total(const std::vector<Line>& lines) {
    return std::accumulate(lines.begin(), lines.end(), 0, [](int sum, const Line& line) {
        int line_total = line.unit_cents * line.quantity;
        if (line_total >= 10000) {
            line_total = line_total * 9 / 10;
        }
        return sum + line_total;
    });
}

}  // namespace stl_style

// 模板元编程是模板子语言里最偏的一支：它在编译期算，不在运行期算。
template <int N>
struct Factorial {
    static constexpr int value = N * Factorial<N - 1>::value;
};

template <>
struct Factorial<0> {
    static constexpr int value = 1;
};

static_assert(Factorial<5>::value == 120);

int main() {
    const c_style::Line c_lines[] = {{1200, 4}, {3500, 3}, {800, 2}};

    std::vector<std::unique_ptr<oo_style::Line>> oo_lines;
    oo_lines.push_back(std::make_unique<oo_style::SimpleLine>(1200, 4));
    oo_lines.push_back(std::make_unique<oo_style::BulkLine>(3500, 3));
    oo_lines.push_back(std::make_unique<oo_style::SimpleLine>(800, 2));

    const std::vector<template_style::Line> t_lines = {{1200, 4}, {3500, 3}, {800, 2}};
    const std::vector<stl_style::Line> s_lines = {{1200, 4}, {3500, 3}, {800, 2}};

    std::cout << "C        " << c_style::total(c_lines, 3) << '\n';
    std::cout << "OO       " << oo_style::total(oo_lines) << '\n';
    std::cout << "template " << template_style::total(t_lines) << '\n';
    std::cout << "STL      " << stl_style::total(s_lines) << '\n';
}
