// 第 5.1 节：函数模板的三类推导，以及数组和函数怎么退化
#include <cstddef>
#include <iostream>
#include <type_traits>

// 用返回值把推导出的 T 带出来，再用 decltype 检查
template <typename T>
struct tag {
    using type = T;
};

template <typename T>
tag<T> by_ref(T&);              // 引用形参：T&

template <typename T>
tag<T> by_const_ref(const T&);  // 带 const 的引用形参：const T&

template <typename T>
tag<T> by_value(T);             // 按值形参：T

template <typename T>
tag<T> by_universal(T&&);       // 通用引用：T&&

template <typename T>
tag<T> by_pointer(T*);          // 指针形参：T*

template <typename T, std::size_t N>
constexpr std::size_t array_size(T (&)[N]) {
    return N;
}

void some_function(int, double) {}

int main() {
    int x = 27;
    const int cx = x;
    const int& rx = x;

    // 情形一：引用或指针形参。先去掉实参的引用性，再匹配；const 保留进 T。
    static_assert(std::is_same_v<decltype(by_ref(x))::type, int>);
    static_assert(std::is_same_v<decltype(by_ref(cx))::type, const int>);
    static_assert(std::is_same_v<decltype(by_ref(rx))::type, const int>);
    static_assert(std::is_same_v<decltype(by_const_ref(cx))::type, int>);
    static_assert(std::is_same_v<decltype(by_pointer(&x))::type, int>);

    const int* px = &x;
    static_assert(std::is_same_v<decltype(by_pointer(px))::type, const int>);

    // 情形二：通用引用。左值实参让 T 推成左值引用，右值实参推成非引用。
    static_assert(std::is_same_v<decltype(by_universal(x))::type, int&>);
    static_assert(std::is_same_v<decltype(by_universal(cx))::type, const int&>);
    static_assert(std::is_same_v<decltype(by_universal(27))::type, int>);

    // 情形三：按值形参。引用、顶层 const、volatile 都被丢掉。
    static_assert(std::is_same_v<decltype(by_value(x))::type, int>);
    static_assert(std::is_same_v<decltype(by_value(cx))::type, int>);
    static_assert(std::is_same_v<decltype(by_value(rx))::type, int>);

    const char name[] = "J. P. Briggs";       // 类型是 const char[13]
    static_assert(std::is_same_v<decltype(by_value(name))::type, const char*>);
    static_assert(std::is_same_v<decltype(by_ref(name))::type, const char[13]>);
    static_assert(array_size(name) == 13);

    // 函数名按值传递时退化成函数指针，按引用传递时保留函数类型
    static_assert(std::is_same_v<decltype(by_value(some_function))::type,
                                 void (*)(int, double)>);
    static_assert(std::is_same_v<decltype(by_ref(some_function))::type,
                                 void (int, double)>);

    std::cout << "x = " << x << "，cx = " << cx << "，rx = " << rx << "\n";
    std::cout << "数组长度 = " << array_size(name)
              << "，首字符 = " << name[0] << "\n";
    std::cout << "按引用保留的数组类型大小 = "
              << sizeof(decltype(by_ref(name))::type) << " 字节\n";
}
