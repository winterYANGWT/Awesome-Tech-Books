// 第 20.3 节：需要两边都能隐式转换时，把非成员函数定义在类模板内部
#include <iostream>

template<class T>
class Ratio {
public:
    Ratio(T numerator = 0, T denominator = 1)
        : numerator_(numerator), denominator_(denominator) {}

    // 隐藏友元：每个 Ratio<T> 实例化时都声明一个具体的非模板函数
    friend Ratio operator*(const Ratio& lhs, const Ratio& rhs) {
        return Ratio(lhs.numerator_ * rhs.numerator_,
                     lhs.denominator_ * rhs.denominator_);
    }

    friend std::ostream& operator<<(std::ostream& os, const Ratio& r) {
        return os << r.numerator_ << '/' << r.denominator_;
    }

private:
    T numerator_;
    T denominator_;
};

int main() {
    Ratio<int> half(1, 2);

    // 2 先隐式转换成 Ratio<int>，再调用上面那个具体的 operator*
    Ratio<int> one = half * 2;
    Ratio<int> two = 2 * half;

    std::cout << "half * 2 = " << one << '\n';
    std::cout << "2 * half = " << two << '\n';
}
