// 第 23.2 节：泛型 lambda 的 auto&& 是转发引用，用 decltype 保留值类别
#include <cstdio>
#include <string>
#include <type_traits>
#include <utility>

// 同一个名字的两个重载：一个收左值，一个收右值
void consume(const std::string& text) {
    std::printf("  左值重载：%s\n", text.c_str());
}
void consume(std::string&& text) {
    std::printf("  右值重载：%s\n", text.c_str());
}

int main() {
    // 按值形参：x 本身是左值，右值实参进来也只会走左值重载
    auto byValue = [](auto x) { consume(x); };

    // 通用引用 + forward：把实参原本的值类别传下去
    auto byForward = [](auto&& x) { consume(std::forward<decltype(x)>(x)); };

    // C++20 可以显式写模板参数列表，直接用 T，不必再借 decltype
    auto byForward20 = []<typename T>(T&& x) { consume(std::forward<T>(x)); };

    std::string name = "orders";

    std::puts("byValue:");
    byValue(name);
    byValue(std::string("临时单号"));

    std::puts("byForward:");
    byForward(name);
    byForward(std::string("临时单号"));

    std::puts("byForward20:");
    byForward20(name);
    byForward20(std::string("临时单号"));

    // 直接看 decltype(x)：左值实参给 T&，右值实参给 T&&
    auto kind = [](auto&& x) -> const char* {
        using Param = decltype(x);
        return std::is_lvalue_reference_v<Param> ? "T&" : "T&&";
    };
    std::printf("decltype(x)：左值实参 -> %s，右值实参 -> %s\n",
                kind(name), kind(std::string("临时单号")));
}
