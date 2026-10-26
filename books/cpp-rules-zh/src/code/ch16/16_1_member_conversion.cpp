// 第 16.1 节：成员 operator+ 的 this 不参与隐式转换，左操作数必须是 Complex
// mode: fail
// expect: invalid operands
#include <iostream>

class Complex {
public:
    // 故意不加 explicit：数值类型允许 double 隐式转换成 Complex
    Complex(double re = 0.0, double im = 0.0) : re_(re), im_(im) {}

    Complex operator+(const Complex& rhs) const {
        return Complex(re_ + rhs.re_, im_ + rhs.im_);
    }

    double real() const { return re_; }
    double imag() const { return im_; }

private:
    double re_;
    double im_;
};

int main() {
    Complex z(1.0, 0.5);
    Complex a = z + 1.0;   // 可以：1.0 是参数列表里的实参，能转成 Complex
    Complex b = 1.0 + z;   // 不行：double 没有成员 operator+
    std::cout << a.real() << ' ' << b.real() << '\n';
}
