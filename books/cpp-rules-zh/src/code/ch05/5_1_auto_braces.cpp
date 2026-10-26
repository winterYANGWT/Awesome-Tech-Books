// 第 5.1 节：auto 推导沿用模板规则，花括号初始化是唯一的例外
#include <initializer_list>
#include <iostream>
#include <type_traits>
#include <vector>

int main() {
    int x = 27;
    const int cx = x;
    const int& rx = x;

    auto x1 = 27;        // 按值：int
    auto x2(27);         // 括号也是按值：int
    auto x3 = {27};      // 拷贝列表初始化：initializer_list<int>
    auto x4{27};         // 直接列表初始化：C++17 起推成 int，不再是列表

    static_assert(std::is_same_v<decltype(x1), int>);
    static_assert(std::is_same_v<decltype(x2), int>);
    static_assert(std::is_same_v<decltype(x3), std::initializer_list<int>>);
    static_assert(std::is_same_v<decltype(x4), int>);

    // 其余情形与模板推导一一对应
    auto ax = x;                 // int
    const auto acx = x;          // const int
    const auto& arx = x;         // const int&
    auto&& uref1 = x;            // 左值初始化：int&
    auto&& uref2 = cx;           // const int&
    auto&& uref3 = 27;           // 右值初始化：int&&

    static_assert(std::is_same_v<decltype(ax), int>);
    static_assert(std::is_same_v<decltype(acx), const int>);
    static_assert(std::is_same_v<decltype(arx), const int&>);
    static_assert(std::is_same_v<decltype(uref1), int&>);
    static_assert(std::is_same_v<decltype(uref2), const int&>);
    static_assert(std::is_same_v<decltype(uref3), int&&>);

    // 数组和函数名在非引用的 auto 下同样退化
    const char name[] = "Ada";
    auto arr = name;
    auto& arr_ref = name;
    static_assert(std::is_same_v<decltype(arr), const char*>);
    static_assert(std::is_same_v<decltype(arr_ref), const char(&)[4]>);

    std::cout << "x1 = " << x1 << "，rx = " << rx
              << "，x3 的大小 = " << x3.size() << "\n";
    std::cout << "arr = " << arr << "，arr_ref[0] = " << arr_ref[0] << "\n";

    // 花括号初始化容器的常见写法：这里先声明再打印，避免只看类型
    std::vector<int> values{1, 2, 3};
    auto copied = values;        // vector<int>，不是 initializer_list
    static_assert(std::is_same_v<decltype(copied), std::vector<int>>);

    // C++17 的 CTAD 是类模板自己的推导，和 auto 的花括号规则不是一回事
    std::vector deduced{1, 2, 3};
    static_assert(std::is_same_v<decltype(deduced), std::vector<int>>);
    std::cout << "copied.size() = " << copied.size()
              << "，deduced.size() = " << deduced.size() << "\n";
}
