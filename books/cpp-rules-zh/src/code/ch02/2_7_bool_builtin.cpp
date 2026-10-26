// 第 2.7 节：bool 必须内建；自定义类型用 explicit operator bool 模仿条件测试
#include <iostream>
#include <type_traits>

class Flag {
public:
    explicit Flag(bool value) : value_(value) {}
    explicit operator bool() const { return value_; }

private:
    bool value_;
};

void check(bool value) { std::cout << "bool " << (value ? "true" : "false") << '\n'; }
void check(int value) { std::cout << "int " << value << '\n'; }

int main() {
    Flag f(true);
    if (f) std::cout << "if (f) works\n";

    check(true);   // 精确匹配 bool
    check(1);      // 精确匹配 int

    // explicit operator bool：能用于 if，但不能悄悄变成 int 或 bool。
    static_assert(!std::is_convertible_v<Flag, int>);
    static_assert(!std::is_convertible_v<Flag, bool>);
    static_assert(std::is_constructible_v<bool, Flag>);

    // 比较表达式的结果类型由语言保证就是 bool。
    static_assert(std::is_same_v<decltype(1 < 2), bool>);
}
