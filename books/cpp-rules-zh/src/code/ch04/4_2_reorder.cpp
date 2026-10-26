// 第 4.2 节：初始化列表的顺序和成员声明顺序不一致时，clang 会警告
// mode: warn
// expect: -Wreorder
#include <iostream>

struct Point {
    int x_;
    int y_;

    Point() : y_(2), x_(1) {}   // 实际先构造 x_，再构造 y_
};

int main() {
    Point p;
    std::cout << p.x_ << "," << p.y_ << "\n";
}
