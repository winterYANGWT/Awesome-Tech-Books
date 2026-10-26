// 第 2.7 节：float 在 1e8 附近已经装不下每个整数
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

int main() {
    float x = 1e8f;
    std::cout << std::boolalpha;
    std::cout << "float digits = " << std::numeric_limits<float>::digits << '\n';
    std::cout << "x = " << std::setprecision(9) << x << '\n';

    // 原书那个 --x 循环若真的跑起来，会在这里永远停住。
    std::cout << "x - 1 == x: " << (x - 1.0f == x) << '\n';

    const float next = std::nextafter(x, std::numeric_limits<float>::infinity());
    std::cout << "ulp at 1e8 = " << (next - x) << '\n';

    double d = 1e8;
    std::cout << "double d - 1 == d: " << (d - 1.0 == d) << '\n';
}
