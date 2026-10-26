// 第 34.2 节：导入方只看见 export 出去的名字
// 导入前定义同名宏，想污染模块内部的 square；模块已经独立编译，不受影响。
#define square(x) 0
import geometry;

#include <iostream>

int main() {
    std::cout << "r=2 面积: " << geo::circle_area(2.0) << '\n';
    geo::Circle c{3.0};
    std::cout << "Circle r=3 面积: " << geo::area(c) << '\n';
    std::cout << "max_sides: " << geo::max_sides << '\n';
#ifdef GEOMETRY_INTERNAL_LIMIT
    std::cout << "模块内部宏泄漏了\n";
#endif
}
