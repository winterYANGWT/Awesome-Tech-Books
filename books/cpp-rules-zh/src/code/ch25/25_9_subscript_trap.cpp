// 第 25.4 节：operator[] 查不到就插入；只查不改要用 find/at/contains
// mode: run
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

template <class M>
concept Subscriptable = requires(M& m, const std::string& k) { m[k]; };
template <class M>
concept FindView = requires(M& m, std::string_view k) { m.find(k); };

// const map 没有 operator[]，因为插入会改变容器
static_assert(!Subscriptable<const std::map<std::string, int>>);
// 默认比较器不透明，find 不接受 string_view
static_assert(!FindView<std::map<std::string, int>>);
// less<> 是透明比较器，find 可以直接收 string_view
static_assert(FindView<std::map<std::string, int, std::less<>>>);

int main() {
    std::map<std::string, int> stock{{"P101", 3}, {"P102", 8}};
    std::cout << "初始 size = " << stock.size() << '\n';

    int& count = stock["P999"];  // 键不存在：静默插入一个值初始化的 0
    std::cout << "stock[\"P999\"] 之后 size = " << stock.size()
              << "，值 = " << count << '\n';

    if (stock.find("P102") != stock.end()) {
        std::cout << "find(\"P102\") 命中，size 仍为 " << stock.size() << '\n';
    }
    std::cout << std::boolalpha << "contains(\"P102\") = " << stock.contains("P102") << '\n';

    try {
        stock.at("P404");
    } catch (const std::out_of_range&) {
        std::cout << "at(\"P404\") 抛出 std::out_of_range，size 仍为 " << stock.size() << '\n';
    }

    const auto& frozen = stock;
    std::cout << "const map 上 at(\"P101\") = " << frozen.at("P101") << '\n';

    // C++14 透明比较器 + C++17 string_view：查找不构造临时 string
    std::map<std::string, int, std::less<>> catalog{{"apple", 1}, {"pear", 2}};
    auto it = catalog.find(std::string_view{"apple"});
    std::cout << "透明比较器 find(\"apple\"sv)："
              << (it != catalog.end() ? "命中" : "没找到") << '\n';
}
