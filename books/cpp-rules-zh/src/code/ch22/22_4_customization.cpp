// 第 22.4 节：三种定制点，以及一个被 ADL 意外拉进来的辅助函数
#include <iostream>
#include <string>
#include <utility>

namespace customer {

struct Invoice {
    int cents;
    // 定制点 1：模板要求一个同名成员函数
    std::string audit() const {
        return "Invoice(" + std::to_string(cents) + " cents)";
    }
};

// 定制点 2：放在类型所在命名空间的非成员 swap，由 ADL 找到
void swap(Invoice& a, Invoice& b) {
    std::cout << "  [customer::swap 被调用]\n";
    std::swap(a.cents, b.cents);
}

// 这个函数不是为库准备的，但它的名字恰好和库的内部辅助函数相同
long long adjust(const Invoice& invoice) {
    return invoice.cents + 1'000'000;
}

}  // namespace customer

namespace billing {

namespace detail {
long long adjust(long long cents) { return cents + 100; }
}
using detail::adjust;  // 让非限定调用能看见库自己的辅助函数

// 定制点 1
template <class T>
std::string audit(const T& t) {
    return t.audit();
}

// 定制点 2
template <class T>
void exchange(T& a, T& b) {
    using std::swap;
    swap(a, b);  // 非限定调用：ADL 会找到 customer::swap
}

// 定制点 3：由用户特化 traits
template <class T>
struct amount_traits;

template <class T>
long long total_cents(const T& t) {
    return amount_traits<T>::to_cents(t);
}

// 意外定制点：非限定调用让实参命名空间里的同名函数参与重载
template <class T>
long long settle_wrong(const T& t) {
    return adjust(t);
}

// 限定调用把 ADL 关掉，只会调用库自己的实现
template <class T>
long long settle_right(const T& t) {
    return detail::adjust(t.cents);
}

}  // namespace billing

namespace billing {
template <>
struct amount_traits<customer::Invoice> {
    static long long to_cents(const customer::Invoice& invoice) {
        return invoice.cents;
    }
};
}  // namespace billing

int main() {
    customer::Invoice first{1200};
    customer::Invoice second{800};

    std::cout << "成员函数定制：" << billing::audit(first) << '\n';

    std::cout << "ADL 定制 swap，交换前：" << first.cents << " / " << second.cents << '\n';
    billing::exchange(first, second);
    std::cout << "交换后：" << first.cents << " / " << second.cents << '\n';

    std::cout << "traits 定制：" << billing::total_cents(first) << '\n';

    std::cout << "意外 ADL：" << billing::settle_wrong(first) << '\n';
    std::cout << "限定调用：" << billing::settle_right(first) << '\n';
}
