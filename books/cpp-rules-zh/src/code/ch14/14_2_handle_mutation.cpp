// 第 14.2 节：const 成员函数交出的引用/指针仍能改对象
#include <cstddef>
#include <iostream>
#include <memory>
#include <span>
#include <vector>

struct Point {
    int x;
    int y;
    void setX(int v) { x = v; }
};

std::ostream& operator<<(std::ostream& os, const Point& p) {
    return os << '(' << p.x << ',' << p.y << ')';
}

class Polyline {
public:
    Polyline() : impl_(std::make_unique<Impl>()) {}

    void add(Point p) { impl_->pts.push_back(p); }

    // 坏：const 只作用在 impl_ 这个指针上，指针指向的 Impl 仍可写。
    Point& at(std::size_t i) const { return impl_->pts.at(i); }

    // 好：只读访问，调用方拿不到写入口。
    const Point& read(std::size_t i) const { return impl_->pts.at(i); }

    // 好：C++20 的只读视图，长度和 const 都明确；但它不延长生命周期。
    std::span<const Point> points() const { return impl_->pts; }

private:
    struct Impl {
        std::vector<Point> pts;
    };
    std::unique_ptr<Impl> impl_;
};

int main() {
    Polyline line;
    line.add({0, 0});
    line.add({10, 5});

    const Polyline& cline = line;
    std::cout << "before " << cline.read(0) << "\n";
    cline.at(0).setX(99);          // 编译通过：一个 const 对象被改了
    std::cout << "after  " << cline.read(0) << "\n";

    int sum = 0;
    for (const Point& p : cline.points()) sum += p.x;
    std::cout << "sum x via span " << sum << "\n";
}
