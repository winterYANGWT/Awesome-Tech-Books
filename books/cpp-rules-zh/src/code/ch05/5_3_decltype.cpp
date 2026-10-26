// 第 5.3 节：decltype 对名字照抄声明类型，对左值表达式加引用
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

struct Widget {
    int id = 7;
};

bool is_valid(const Widget& w) { return w.id > 0; }

int global = 0;

// 括号让表达式不再是“名字”，返回类型从 int 变成 int&
decltype(auto) ref_to_global() { return (global); }

// auto 返回类型用模板推导，会丢掉引用；decltype(auto) 保留
template <typename Container>
auto front_copy(Container& c) {
    return c[0];
}

template <typename Container>
decltype(auto) front_ref(Container& c) {
    return c[0];
}

template <typename Container>
auto front_trailing(Container& c) -> decltype(c[0]) {
    return c[0];
}

// C++11 的 alias template；typedef 做不到这件事
template <typename T>
using Row = std::vector<T>;

template <typename T>
struct OldRow {
    typedef std::vector<T> type;
};

template <typename T>
void build_rows() {
    Row<T> first;                    // 直接就是类型名
    typename OldRow<T>::type second; // 依赖名，必须加 typename
    first.push_back(T{});
    second.push_back(T{});
    std::cout << "Row<T> 与 OldRow<T>::type 大小："
              << first.size() << " / " << second.size() << "\n";
}

int main() {
    const int i = 0;
    int x = 1;
    const Widget w;
    std::vector<int> data{1, 2, 3};

    // 名字：decltype 给出声明类型，保留 const 和引用
    static_assert(std::is_same_v<decltype(i), const int>);
    static_assert(std::is_same_v<decltype(w), const Widget>);
    static_assert(std::is_same_v<decltype(is_valid), bool(const Widget&)>);
    static_assert(std::is_same_v<decltype(Widget::id), int>);

    // 非名字的左值表达式：类型 T 变成 T&
    static_assert(std::is_same_v<decltype((i)), const int&>);
    static_assert(std::is_same_v<decltype((x)), int&>);
    static_assert(std::is_same_v<decltype(data[0]), int&>);

    // 将亡值给 T&&，纯右值给 T
    static_assert(std::is_same_v<decltype(std::move(x)), int&&>);
    static_assert(std::is_same_v<decltype(x + 1), int>);

    // decltype(auto) 把 decltype 的规则用到变量上
    decltype(auto) plain = x;        // int
    decltype(auto) alias = (x);      // int&
    static_assert(std::is_same_v<decltype(plain), int>);
    static_assert(std::is_same_v<decltype(alias), int&>);
    alias = 42;

    const Widget& cw = w;
    decltype(auto) keep_const = cw;  // const Widget&
    static_assert(std::is_same_v<decltype(keep_const), const Widget&>);

    // 返回类型：auto 丢引用，decltype(auto) 和尾置返回类型保留
    static_assert(std::is_same_v<decltype(front_copy(data)), int>);
    static_assert(std::is_same_v<decltype(front_ref(data)), int&>);
    static_assert(std::is_same_v<decltype(front_trailing(data)), int&>);
    front_ref(data) = 30;

    static_assert(std::is_same_v<decltype(ref_to_global()), int&>);
    ref_to_global() = 9;

    // C++14 的 _t 别名就是 alias template 的标准库应用
    static_assert(std::is_same_v<std::remove_const_t<const int>, int>);
    static_assert(std::is_same_v<std::add_lvalue_reference_t<int>, int&>);

    build_rows<std::string>();

    std::cout << "x = " << x << "，global = " << global
              << "，data[0] = " << data[0] << "\n";
    std::cout << "cw.id = " << keep_const.id << "\n";
}
