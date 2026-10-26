// 第 5.4 节：四组引用折叠，以及它在模板、auto、别名和 decltype 里的样子
#include <iostream>
#include <type_traits>
#include <utility>

template <typename T>
struct tag {
    using type = T;
};

template <typename T>
tag<T> deduce(T&&);   // 通用引用：把左值/右值编码进 T

template <typename T>
using AddLRef = T&;

template <typename T>
using AddRRef = T&&;

template <typename T>
struct Holder {
    using RvalueRefToT = T&&;
};

int main() {
    // 四种组合：只要有一个左值引用，结果就是左值引用
    static_assert(std::is_same_v<AddLRef<int&>, int&>);    // & + &
    static_assert(std::is_same_v<AddLRef<int&&>, int&>);   // & + &&
    static_assert(std::is_same_v<AddRRef<int&>, int&>);    // && + &
    static_assert(std::is_same_v<AddRRef<int&&>, int&&>);  // && + &&

    // 模板实例化：左值让 T 变成 int&，代入 T&& 后折叠成 int&
    int x = 1;
    static_assert(std::is_same_v<decltype(deduce(x))::type, int&>);
    static_assert(std::is_same_v<decltype(deduce(2))::type, int>);

    // 别名模板里也折叠：Holder<int&>::RvalueRefToT 实际是 int&
    static_assert(std::is_same_v<Holder<int&>::RvalueRefToT, int&>);
    static_assert(std::is_same_v<Holder<int>::RvalueRefToT, int&&>);

    // decltype 里也会遇到引用之引用
    using L = int&;
    using R = int&&;
    static_assert(std::is_same_v<decltype(static_cast<L&&>(x)), int&>);
    static_assert(std::is_same_v<decltype(static_cast<R&&>(x)), int&&>);

    // 标准库 trait 同样按折叠规则处理
    static_assert(std::is_same_v<std::add_lvalue_reference_t<int&&>, int&>);

    // 用起来看：deduce 返回的 tag 不影响 x，改的是同一对象
    x = 10;
    std::cout << "x = " << x
              << "，Holder<int&>::RvalueRefToT 是左值引用 = "
              << std::is_lvalue_reference_v<Holder<int&>::RvalueRefToT> << "\n";

    std::cout << std::boolalpha
              << "std::add_lvalue_reference_t<int&&> 是 int&："
              << std::is_same_v<std::add_lvalue_reference_t<int&&>, int&>
              << "\n";
}
