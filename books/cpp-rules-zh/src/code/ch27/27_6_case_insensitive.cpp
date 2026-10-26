// 第 27.6 节：用 mismatch 和 lexicographical_compare 拼出忽略大小写的比较
#include <algorithm>
#include <cctype>
#include <iostream>
#include <set>
#include <string>
#include <utility>

int ci_char_compare(char a, char b) {
    // tolower 的实参必须先转成 unsigned char，char 是否有符号由实现决定。
    int la = std::tolower(static_cast<unsigned char>(a));
    int lb = std::tolower(static_cast<unsigned char>(b));
    if (la < lb) return -1;
    if (la > lb) return 1;
    return 0;
}

// strcmp 风格的三态返回：负数、0、正数。
int ci_compare(const std::string& a, const std::string& b) {
    const bool a_is_shorter = a.size() <= b.size();
    const std::string& shorter = a_is_shorter ? a : b;
    const std::string& longer = a_is_shorter ? b : a;

    // mismatch 在谓词返回 false 时停下，所以这里判断“两个字符忽略大小写后相等”。
    auto p = std::mismatch(shorter.begin(), shorter.end(), longer.begin(),
                           [](char x, char y) { return ci_char_compare(x, y) == 0; });

    int cmp;
    if (p.first == shorter.end())
        cmp = (p.second == longer.end()) ? 0 : -1;  // 短的是长的前缀
    else
        cmp = ci_char_compare(*p.first, *p.second);

    // 如果交换过实参，返回值要取反。
    return a_is_shorter ? cmp : -cmp;
}

bool ci_char_less(char a, char b) {
    return std::tolower(static_cast<unsigned char>(a))
         < std::tolower(static_cast<unsigned char>(b));
}

// 严格弱序：可以直接作为关联容器的比较器。
bool ci_less(const std::string& a, const std::string& b) {
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), ci_char_less);
}

struct CiLess {
    bool operator()(const std::string& a, const std::string& b) const {
        return ci_less(a, b);
    }
};

int main() {
    std::cout << "ci_compare(Hello, hello)=" << ci_compare("Hello", "hello") << '\n';
    std::cout << "ci_compare(apple, Apricot)=" << ci_compare("apple", "Apricot") << '\n';
    std::cout << "ci_compare(Banana, apple)=" << ci_compare("Banana", "apple") << '\n';
    std::cout << "ci_compare(prefix, prefix-long)=" << ci_compare("prefix", "prefix-long") << '\n';

    std::set<std::string, CiLess> words;
    auto first = words.insert("Apple");
    auto second = words.insert("apple");
    words.insert("Banana");
    words.insert("apricot");
    std::cout << "set 大小=" << words.size()
              << "，第二次插入 Apple 成功？" << second.second
              << "，第一次插入成功？" << first.second << '\n';
    std::cout << "set 顺序：";
    for (const auto& w : words) std::cout << w << ' ';
    std::cout << '\n';
}
