// 第 27.1 节：只需要多少顺序，就付多少代价
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

struct Applicant {
    std::string name;
    int score;
};

// 完整的排序规则：分数高的在前，同分按名字，保证输出可复现。
bool better(const Applicant& a, const Applicant& b) {
    if (a.score != b.score) return a.score > b.score;
    return a.name < b.name;
}

// 只比分数：同分元素的先后由算法决定，只有 stable_sort 承诺保持原序。
bool score_only(const Applicant& a, const Applicant& b) {
    return a.score > b.score;
}

void show(const std::string& label, std::vector<Applicant> v) {
    std::sort(v.begin(), v.end(), better);  // 只为打印稳定，和被测算法无关
    std::cout << label;
    for (const auto& a : v) std::cout << ' ' << a.name << '(' << a.score << ')';
    std::cout << '\n';
}

int main() {
    std::vector<Applicant> all{
        {"Ada", 91}, {"Bob", 78}, {"Cyd", 91}, {"Dee", 65}, {"Eve", 84},
        {"Fay", 78}, {"Gus", 95}, {"Hal", 70}, {"Ivy", 84}, {"Jon", 59},
    };

    // partition：只把区间分成“通过”和“未通过”两组，不排组内顺序。
    auto passed = all;
    auto first_low = std::partition(passed.begin(), passed.end(),
                                    [](const Applicant& a) { return a.score >= 80; });
    std::vector<Applicant> pass(passed.begin(), first_low);
    std::vector<Applicant> fail(first_low, passed.end());
    std::cout << "partition(>=80)：" << pass.size() << " 人通过，"
              << fail.size() << " 人未通过\n";
    show("  通过：", pass);
    show("  未通过：", fail);

    // nth_element：只保证下标 2 上的元素是全序中的第 3 个，前 3 个是哪些确定，
    // 但它们内部的顺序不确定，所以这里只按集合打印。
    auto top = all;
    std::nth_element(top.begin(), top.begin() + 2, top.end(), better);
    show("nth_element 前 3（集合对，内部无序）：",
         std::vector<Applicant>(top.begin(), top.begin() + 3));

    // partial_sort：前 3 个不仅选对，而且已经按比较器排好。
    auto part = all;
    std::partial_sort(part.begin(), part.begin() + 3, part.end(), better);
    show("partial_sort 前 3（按序）：",
         std::vector<Applicant>(part.begin(), part.begin() + 3));

    // sort：整个区间有序，但不稳定；这里用带名字的 better，输出才确定。
    auto sorted = all;
    std::sort(sorted.begin(), sorted.end(), better);
    show("sort 全序：", sorted);

    // stable_sort：只比分数时，同分的 Ada、Cyd 保持输入里的先后。
    auto stable = all;
    std::stable_sort(stable.begin(), stable.end(), score_only);
    std::cout << "stable_sort 前 4（同分保持原序）：";
    for (std::size_t i = 0; i != 4; ++i)
        std::cout << ' ' << stable[i].name << '(' << stable[i].score << ')';
    std::cout << '\n';
}
