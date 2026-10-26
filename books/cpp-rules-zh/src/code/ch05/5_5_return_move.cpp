// 第 5.5 节：返回局部对象时写 std::move 会妨碍 NRVO
// flags: -Wno-pessimizing-move
#include <iostream>
#include <utility>

struct Report {
    static int copies;
    static int moves;

    int lines = 0;

    Report() = default;
    Report(const Report& other) : lines(other.lines) { ++copies; }
    Report(Report&& other) noexcept : lines(other.lines) {
        other.lines = -1;
        ++moves;
    }
    Report& operator=(const Report&) = default;
    Report& operator=(Report&&) = default;
};

int Report::copies = 0;
int Report::moves = 0;

Report make_direct() {
    Report report;
    report.lines = 10;
    return report;            // 满足 NRVO 条件，编译器可以直接构造在返回值里
}

Report make_with_move() {
    Report report;
    report.lines = 10;
    return std::move(report); // 返回的是引用，NRVO 条件不成立，必须移动
}

Report make_from_param(Report report) {
    report.lines = 10;
    return report;            // 形参不满足 NRVO，但返回时按右值处理
}

int main() {
    std::cout << "--- 直接返回局部对象 ---\n";
    Report a = make_direct();
    std::cout << "lines = " << a.lines << "，拷贝 = " << Report::copies
              << "，移动 = " << Report::moves << "\n";

    Report::copies = Report::moves = 0;
    std::cout << "--- 返回 std::move(局部对象) ---\n";
    Report b = make_with_move();
    std::cout << "lines = " << b.lines << "，拷贝 = " << Report::copies
              << "，移动 = " << Report::moves << "\n";

    Report::copies = Report::moves = 0;
    std::cout << "--- 按值形参被返回 ---\n";
    Report c = make_from_param(Report{});
    std::cout << "lines = " << c.lines << "，拷贝 = " << Report::copies
              << "，移动 = " << Report::moves << "\n";
}
