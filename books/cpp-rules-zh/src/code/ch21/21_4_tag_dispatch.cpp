// 第 21.4 节：tag dispatch 用一个标签参数决定走哪个重载
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

std::vector<std::string> names;

std::string nameFromIdx(int idx) {
    return "name-" + std::to_string(idx);
}

// 非整型：转发到容器里直接构造字符串。
template <typename T>
void logAndAddImpl(T&& name, std::false_type) {
    names.emplace_back(std::forward<T>(name));
}

// 整型：按索引查出名字，再走上面那条路。
void logAndAddImpl(int idx, std::true_type) {
    names.emplace_back(nameFromIdx(idx));
}

template <typename T>
void logAndAdd(T&& name) {
    // T 可能是 int&，所以先用 remove_cvref_t 去掉引用和 const。
    using Raw = std::remove_cvref_t<T>;
    logAndAddImpl(std::forward<T>(name), std::bool_constant<std::is_integral_v<Raw>>{});
}

int main() {
    const std::string petName("Darla");

    logAndAdd(petName);                  // 左值 string：拷贝
    logAndAdd(std::string("Persephone")); // 右值 string：移动
    logAndAdd("Patty Dog");              // 字符数组：在 vector 里直接构造 string
    logAndAdd(42);                       // int 右值：索引
    short index = 7;
    logAndAdd(index);                    // short 左值：先去掉引用，仍按索引处理

    for (const auto& name : names) {
        std::cout << name << '\n';
    }

    static_assert(!std::is_integral_v<int&>);
    static_assert(std::is_integral_v<std::remove_cvref_t<int&>>);
}
