// 第 2.2 节：引用表示“一定有对象”，指针表示“可能没有”
#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>

struct Theme {
    std::string name;
};

// document 一定存在，用引用；theme 可以没有，用指针。
void render(const std::string& document, const Theme* theme) {
    std::cout << "render " << document;
    if (theme != nullptr) {
        std::cout << " with theme " << theme->name;
    } else {
        std::cout << " with default theme";
    }
    std::cout << '\n';
}

// 重载决议：0 是 int，nullptr 是 std::nullptr_t。
void select(int) { std::cout << "select(int)\n"; }
void select(bool) { std::cout << "select(bool)\n"; }
void select(void*) { std::cout << "select(void*)\n"; }

template <class T>
struct TypeName;

template <>
struct TypeName<int> {
    static constexpr const char* value = "int";
};

template <>
struct TypeName<std::nullptr_t> {
    static constexpr const char* value = "std::nullptr_t";
};

template <class T>
void relay(T) {
    std::cout << "relay deduced " << TypeName<T>::value << '\n';
}

int main() {
    Theme dark{"dark"};
    render("report.txt", &dark);
    render("report.txt", nullptr);

    select(0);        // int：0 首先是整数
    select(nullptr);  // void*：nullptr 不是整数

    relay(0);
    relay(nullptr);

    static_assert(std::is_same_v<decltype(nullptr), std::nullptr_t>);
    static_assert(!std::is_convertible_v<std::nullptr_t, int>);
}
