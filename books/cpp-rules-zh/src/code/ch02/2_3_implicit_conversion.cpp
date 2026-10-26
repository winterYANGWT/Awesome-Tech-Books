// 第 2.3 节：两种隐式转换让错误的表达式静默通过
#include <cstddef>
#include <iostream>
#include <vector>

class Fraction {
public:
    Fraction(int numerator, int denominator)
        : num_(numerator), den_(denominator) {}

    // 转换运算符：任何需要 double 的地方都会悄悄调用它。
    operator double() const { return static_cast<double>(num_) / den_; }

private:
    int num_;
    int den_;
};

class Buffer {
public:
    // 单参构造函数：允许 int 隐式变成一个 Buffer。
    Buffer(std::size_t n) : data_(n, 0) {}

    int& operator[](std::size_t i) { return data_[i]; }
    int operator[](std::size_t i) const { return data_[i]; }
    std::size_t size() const { return data_.size(); }

    bool operator==(const Buffer& other) const { return data_ == other.data_; }

private:
    std::vector<int> data_;
};

int main() {
    Fraction half(1, 2);
    // 本意可能是打印 "1/2"，但因为没有 operator<<，编译器找到了 operator double。
    std::cout << "cout << half -> " << half << '\n';

    Buffer a(4), b(4);
    for (std::size_t i = 0; i < a.size(); ++i) a[i] = static_cast<int>(i);
    b[3] = 3;

    // 漏写下标的 a == b[3] 没有报错：b[3] 先被转成 Buffer(3)。
    // 比较的是 a 和“3 个 0 的临时 Buffer”，不是两个元素。
    std::cout << std::boolalpha;
    std::cout << "a == b[3] -> " << (a == b[3]) << '\n';
    std::cout << "a[3] == b[3] -> " << (a[3] == b[3]) << '\n';
}
