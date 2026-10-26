// 第 16.1 节：非成员 operator+ 让左右两侧平等地参与隐式转换
#include <iostream>

class Complex {
public:
    // 数值类型允许 double 隐式转换；每次转换都会记一笔，用来观察临时对象
    Complex(double re = 0.0, double im = 0.0) : re_(re), im_(im) { ++conversions_; }

    Complex& operator+=(const Complex& rhs) {
        re_ += rhs.re_;
        im_ += rhs.im_;
        return *this;
    }

    Complex& operator-=(const Complex& rhs) {
        re_ -= rhs.re_;
        im_ -= rhs.im_;
        return *this;
    }

    double real() const { return re_; }
    double imag() const { return im_; }
    static int conversions() { return conversions_; }

private:
    double re_;
    double im_;
    static inline int conversions_ = 0;
};

// 独立运算符只写一次逻辑：复制左侧，再调用复合赋值。
// 第一个参数按值接收，左值复制、纯右值直接初始化，交给编译器选最省的方式。
Complex operator+(Complex lhs, const Complex& rhs) { return lhs += rhs; }
Complex operator-(Complex lhs, const Complex& rhs) { return lhs -= rhs; }

std::ostream& operator<<(std::ostream& os, const Complex& z) {
    return os << '(' << z.real() << ',' << z.imag() << ')';
}

int main() {
    Complex z(1.0, 0.5);

    int before = Complex::conversions();
    Complex left = z + 1.0;
    int after_left = Complex::conversions();
    Complex right = 1.0 + z;
    int after_right = Complex::conversions();

    std::cout << "z + 1.0 = " << left << "，转换 " << after_left - before << " 次\n";
    std::cout << "1.0 + z = " << right << "，转换 " << after_right - after_left << " 次\n";
    std::cout << "1.0 + (z + 2.0) = " << 1.0 + (z + 2.0) << '\n';

    // 返回非 const 的 T 时，a + b = c 仍能编译：赋值是对临时对象调用成员 operator=。
    // 返回 const T 能挡住它，却也会抑制移动，所以现代写法接受这个瑕疵。
    static_assert(requires(Complex a, Complex b) { a + b = b; });

    Complex sum = z;
    sum += Complex(1.0, 2.0);
    sum += 3.0;                       // += 的右操作数也能隐式转换
    std::cout << "sum = " << sum << '\n';
}
