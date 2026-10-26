// 第 25.2 节：C++20 起无捕获 lambda 的闭包类型可默认构造，能直接当比较器类型
// mode: run
#include <concepts>
#include <iostream>
#include <set>
#include <string>

int main() {
    // lambda 出现在 decltype 里（C++20），闭包类型直接做模板实参
    using TextLess = decltype([](const std::string* a, const std::string* b) {
        return *a < *b;
    });
    static_assert(std::default_initializable<TextLess>);

    std::set<std::string*, TextLess> words;  // 不必再传比较器对象
    std::string pear = "Pear";
    std::string apple = "Apple";
    std::string orange = "Orange";
    words.insert(&pear);
    words.insert(&apple);
    words.insert(&orange);

    std::cout << "lambda 比较器按内容排序：";
    for (auto* p : words) {
        std::cout << ' ' << *p;
    }
    std::cout << '\n';

    // 有捕获的闭包没有默认构造函数，仍然得把对象传给容器构造函数
    int bonus = 10;
    auto by_size = [bonus](const std::string* a, const std::string* b) {
        return a->size() + static_cast<std::size_t>(bonus) <
               b->size() + static_cast<std::size_t>(bonus);
    };
    static_assert(!std::default_initializable<decltype(by_size)>);
    std::cout << std::boolalpha << "捕获 lambda 可默认构造："
              << std::default_initializable<decltype(by_size)> << '\n';
    std::cout << "捕获版本比较 Apple 和 Pear："
              << by_size(&apple, &pear) << '\n';
}
