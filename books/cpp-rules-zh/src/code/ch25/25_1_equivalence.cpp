// 第 25.1 节：相等看 ==，等价看比较器；成员 find 与 std::find 给出不同答案
// mode: run
// expect: std::find(..., "content-TYPE")：没找到
#include <algorithm>
#include <concepts>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
#include <string_view>

// HTTP 头字段名不区分大小写；比较器只回答“a 是否排在 b 前面”
struct HeaderLess {
    using is_transparent = void;  // C++14：允许 find/erase 直接接收 string_view

    static char lower(char c) {
        return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
    }

    bool operator()(std::string_view a, std::string_view b) const {
        auto fold_less = [](char x, char y) { return HeaderLess::lower(x) < HeaderLess::lower(y); };
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), fold_less);
    }
};

// 概念只检查“能用这两个类型调用、返回 bool”，不检查运行时是否真的传递
static_assert(std::strict_weak_order<HeaderLess, std::string, std::string>);

int main() {
    std::set<std::string> sensitive;
    sensitive.insert("Content-Type");
    sensitive.insert("content-type");
    std::cout << "大小写敏感 set：两个写法是两个元素，size = " << sensitive.size() << '\n';

    std::set<std::string, HeaderLess> headers;
    auto first = headers.insert("Content-Type");
    auto second = headers.insert("content-type");
    headers.insert("CONTENT-TYPE");
    std::cout << "忽略大小写 set：插入三次变体后 size = " << headers.size()
              << "，迭代器指向 " << *headers.begin() << '\n';
    std::cout << std::boolalpha;
    std::cout << "第一次插入的 insert().second = " << first.second << '\n';
    std::cout << "第二次插入的 insert().second = " << second.second << '\n';

    // 关联容器的成员函数走树，用比较器判断等价
    auto member = headers.find(std::string_view{"content-TYPE"});
    std::cout << "成员 find(\"content-TYPE\")："
              << (member != headers.end() ? *member : "没找到") << '\n';

    // std::find 是线性扫描，用 operator== 判断相等
    auto linear = std::find(headers.begin(), headers.end(), std::string("content-TYPE"));
    std::cout << "std::find(..., \"content-TYPE\")："
              << (linear != headers.end() ? *linear : "没找到") << '\n';

    const std::string a = "Content-Type";
    const std::string b = "content-type";
    HeaderLess comp;
    std::cout << "comp(a,b) = " << comp(a, b) << "，comp(b,a) = " << comp(b, a)
              << "，!comp(a,b) && !comp(b,a) = " << (!comp(a, b) && !comp(b, a)) << '\n';
    std::cout << "a == b = " << (a == b) << '\n';

    auto range = headers.equal_range(std::string_view{"CONTENT-type"});
    std::cout << "equal_range 的元素个数：" << std::distance(range.first, range.second) << '\n';
}
