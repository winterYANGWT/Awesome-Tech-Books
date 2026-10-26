// 第 5.1 节：花括号不能推导出唯一的元素类型，auto 返回类型也不推 initializer_list
// mode: fail
#include <initializer_list>

auto mixed = {1, 2, 3.0};   // 元素类型不一致，无法推出 initializer_list<T>
auto pair{1, 2};            // 直接列表初始化多个元素：C++17 起不允许

auto make_list() {          // auto 返回类型使用模板推导，不从花括号推
    return {1, 2, 3};
}

template <typename T>
void take(T) {}

void abbreviated(auto) {}   // C++20 的简写函数模板，同样按模板推导

int main() {
    take({1, 2, 3});        // 形参不是 initializer_list，花括号是非推导语境
    abbreviated({1, 2, 3}); // 简写模板也不例外
}
