// 第 27.2 节：先看区间是否有序，再按需要的信息选查找；成员版本优先
#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

struct User {
    std::string name;
    int id;
};

bool operator==(const User& a, const User& b) {
    return a.id == b.id && a.name == b.name;
}

struct ById {
    bool operator()(const User& a, const User& b) const { return a.id < b.id; }
};

template <class It>
void show_names(const std::string& label, It first, It last) {
    std::cout << label;
    for (; first != last; ++first) std::cout << ' ' << first->name << '(' << first->id << ')';
    std::cout << '\n';
}

int main() {
    std::cout << std::boolalpha;

    // 无序区间：只能线性查找，命中就停。
    std::vector<int> raw{5, 1, 4, 2, 3};
    std::cout << "无序 raw：is_sorted=" << std::is_sorted(raw.begin(), raw.end())
              << "，std::find(4) "
              << (std::find(raw.begin(), raw.end(), 4) != raw.end() ? "命中" : "未命中") << '\n';
    std::sort(raw.begin(), raw.end());
    std::cout << "排序后 raw：is_sorted=" << std::is_sorted(raw.begin(), raw.end())
              << "，binary_search(4)=" << std::binary_search(raw.begin(), raw.end(), 4) << '\n';

    // 降序区间：排序和查找必须用同一个比较器。
    std::vector<int> desc{9, 7, 5, 3, 1};
    std::cout << "降序 desc：默认 is_sorted=" << std::is_sorted(desc.begin(), desc.end())
              << "，用 greater is_sorted="
              << std::is_sorted(desc.begin(), desc.end(), std::greater<>{})
              << "，binary_search(5, greater)="
              << std::binary_search(desc.begin(), desc.end(), 5, std::greater<>{}) << '\n';

    // 有序 users：按 id 排序，两个 42 号是等价元素，但名字不同。
    std::vector<User> users{{"Bob", 7}, {"Ada", 42}, {"Cyd", 42}, {"Dee", 99}};
    User probe{"Zoe", 42};

    std::cout << "有序 users 上 binary_search(probe)="
              << std::binary_search(users.begin(), users.end(), probe, ById{}) << '\n';

    auto it = std::lower_bound(users.begin(), users.end(), probe, ById{});
    bool equal_by_operator = (*it == probe);
    bool equivalent = !ById{}(*it, probe) && !ById{}(probe, *it);
    std::cout << "lower_bound 指向 " << it->name << '(' << it->id << ')'
              << "；*it == probe ？" << equal_by_operator
              << "；按 ById 等价？" << equivalent << '\n';

    auto [lo, hi] = std::equal_range(users.begin(), users.end(), probe, ById{});
    std::cout << "equal_range 覆盖 " << std::distance(lo, hi) << " 个元素：";
    show_names("", lo, hi);

    std::cout << "对 users 用 std::find(probe)："
              << (std::find(users.begin(), users.end(), probe) != users.end() ? "找到" : "未找到")
              << '\n';

    // set 的成员 find 按比较器找等价元素；std::find 按 operator== 找完全相同的对象。
    std::set<User, ById> by_id(users.begin(), users.end());
    std::cout << "set 大小=" << by_id.size()
              << "；成员 find 命中 " << by_id.find(probe)->name
              << "；std::find "
              << (std::find(by_id.begin(), by_id.end(), probe) != by_id.end() ? "找到" : "未找到")
              << '\n';

    // map 的成员 count 只看 key；std::count 比较整个 pair。
    std::map<int, std::string> names{{42, "Ada"}, {7, "Bob"}};
    std::pair<const int, std::string> whole{42, "Zoe"};
    std::cout << "map 成员 count(42)=" << names.count(42)
              << "；std::count 整个 pair=" << std::count(names.begin(), names.end(), whole)
              << '\n';
}
