// 第 20.5 节：成员模板只扩展转换，同类型拷贝仍由编译器生成的版本负责
#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <stdexcept>

template<class T, std::size_t N>
class FixedVector {
public:
    FixedVector() = default;

    FixedVector(std::initializer_list<T> init) {
        std::copy_n(init.begin(), std::min(init.size(), N), data_);
    }

    // 同类型拷贝：显式要求编译器生成，意图比“不写”更清楚
    FixedVector(const FixedVector&) = default;
    FixedVector& operator=(const FixedVector&) = default;

    // 跨元素类型、跨长度的转换构造
    template<class U, std::size_t M>
    FixedVector(const FixedVector<U, M>& other) {
        ++template_copies;
        std::copy(other.begin(), other.begin() + std::min(N, M), data_);
    }

    template<class U, std::size_t M>
    FixedVector& operator=(const FixedVector<U, M>& other) {
        ++template_assigns;
        std::copy(other.begin(), other.begin() + std::min(N, M), data_);
        return *this;
    }

    T* begin() { return data_; }
    const T* begin() const { return data_; }
    T* end() { return data_ + N; }
    const T* end() const { return data_ + N; }

    T& operator[](std::size_t i) { return data_[i]; }
    const T& operator[](std::size_t i) const { return data_[i]; }

    static int template_copies;
    static int template_assigns;

private:
    T data_[N]{};
};

template<class T, std::size_t N>
int FixedVector<T, N>::template_copies = 0;

template<class T, std::size_t N>
int FixedVector<T, N>::template_assigns = 0;

struct Element {
    int value = 0;

    Element() = default;
    Element(int v) : value(v) {}
    Element(const Element&) = default;

    Element& operator=(const Element& other) {
        if (other.value == 13) {
            throw std::runtime_error("cannot store 13");
        }
        value = other.value;
        return *this;
    }
};

int main() {
    FixedVector<int, 4> a{1, 2, 3, 4};
    FixedVector<int, 4> b(a);  // 精确同类型：走隐式拷贝构造
    std::cout << "same-type template copies = "
              << FixedVector<int, 4>::template_copies << '\n';

    FixedVector<short, 6> wide{1, 2, 3, 4, 5, 6};
    FixedVector<int, 4> narrow(wide);  // 不同元素类型和长度：走成员模板
    std::cout << "narrow = " << narrow[0] << ',' << narrow[1] << ','
              << narrow[2] << ',' << narrow[3]
              << ", template copies = " << FixedVector<int, 4>::template_copies << '\n';

    FixedVector<int, 4> assigned;
    assigned = wide;  // 成员模板赋值
    std::cout << "cross-type template assigns = "
              << FixedVector<int, 4>::template_assigns << '\n';

    assigned = b;  // 精确同类型：隐式拷贝赋值，模板计数不变
    std::cout << "after same-type assign = "
              << FixedVector<int, 4>::template_assigns << '\n';

    // 逐元素赋值在第三个元素上抛出，前两个已经改了：只满足基本保证
    FixedVector<Element, 4> src;
    src[0].value = 1;
    src[1].value = 2;
    src[2].value = 13;  // 赋值时抛出，构造时不能走 operator=
    src[3].value = 4;

    FixedVector<Element, 4> dst;
    for (int i = 0; i < 4; ++i) {
        dst[i].value = 9;
    }
    try {
        dst = src;
    } catch (const std::exception& e) {
        std::cout << "exception: " << e.what() << '\n';
    }
    std::cout << "dst = " << dst[0].value << ',' << dst[1].value << ','
              << dst[2].value << ',' << dst[3].value << '\n';
}
