// 第 14.3 节：重载决议先于访问检查，选中的是私有重载
// mode: fail
// expect: private
#include <complex>
#include <iostream>

class Calc {
public:
    double twice(double d) const { return 2 * d; }

private:
    int twice(int i) const { return 2 * i; }
    std::complex<float> twice(std::complex<float> c) const { return c * 2.0f; }
};

int main() {
    Calc c;
    std::cout << c.twice(21) << "\n";   // int 精确匹配私有重载
}
