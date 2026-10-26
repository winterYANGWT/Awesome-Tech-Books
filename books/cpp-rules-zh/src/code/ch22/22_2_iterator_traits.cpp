// 第 22.2 节：traits 提供类型信息，重载在编译期选实现
#include <forward_list>
#include <iostream>
#include <iterator>
#include <list>
#include <type_traits>
#include <vector>

// 三个 worker 只在最后一个参数（迭代器类别标签）上不同。
template <class It, class Dist>
void do_advance(It& it, Dist d, std::random_access_iterator_tag) {
    std::cout << "  [随机访问] it += d\n";
    it += d;
}

template <class It, class Dist>
void do_advance(It& it, Dist d, std::bidirectional_iterator_tag) {
    std::cout << "  [双向] 循环 ++/--\n";
    if (d >= 0)
        while (d--) ++it;
    else
        while (d++) --it;
}

template <class It, class Dist>
void do_advance(It& it, Dist d, std::input_iterator_tag) {
    std::cout << "  [单向] 只往前循环\n";
    if (d < 0) throw std::out_of_range("input iterator 不能后退");
    while (d--) ++it;
}

// 主函数：从 traits 取出类别，造一个标签对象让重载决议挑选 worker。
template <class It, class Dist>
void advance_by(It& it, Dist d) {
    using category = typename std::iterator_traits<It>::iterator_category;
    do_advance(it, d, category{});
}

// C++20 可以不再写 worker 重载，直接在函数里按 concept 分支。
template <class It, class Dist>
void advance_by20(It& it, Dist d) {
    if constexpr (std::random_access_iterator<It>) {
        std::cout << "  [C++20 随机访问] it += d\n";
        it += d;
    } else if constexpr (std::bidirectional_iterator<It>) {
        std::cout << "  [C++20 双向] 循环 ++/--\n";
        if (d >= 0)
            while (d--) ++it;
        else
            while (d++) --it;
    } else {
        std::cout << "  [C++20 单向] 只往前循环\n";
        while (d--) ++it;
    }
}

// traits 的 value_type 对指针同样有效，所以这个函数能直接收 int*。
template <class It>
auto sum_all(It first, It last) {
    using value_type = typename std::iterator_traits<It>::value_type;
    value_type total{};
    for (; first != last; ++first) total += *first;
    return total;
}

static_assert(std::is_same_v<
              std::iterator_traits<std::vector<int>::iterator>::iterator_category,
              std::random_access_iterator_tag>);
static_assert(std::is_same_v<
              std::iterator_traits<std::list<int>::iterator>::iterator_category,
              std::bidirectional_iterator_tag>);
static_assert(std::is_same_v<std::iterator_traits<const int*>::value_type, int>);

int main() {
    std::vector<int> readings{10, 20, 30, 40, 50};
    auto vit = readings.begin();
    std::cout << "vector：\n";
    advance_by(vit, 3);
    std::cout << "  结果 = " << *vit << '\n';

    std::list<int> pending{10, 20, 30, 40, 50};
    auto lit = pending.begin();
    std::cout << "list：\n";
    advance_by(lit, 3);
    std::cout << "  结果 = " << *lit << '\n';

    std::forward_list<int> stream{10, 20, 30, 40, 50};
    auto fit = stream.begin();
    std::cout << "forward_list（forward_iterator_tag 派生自 input_iterator_tag）：\n";
    advance_by(fit, 3);
    std::cout << "  结果 = " << *fit << '\n';

    const int* raw = readings.data();
    std::cout << "const int*：\n";
    advance_by(raw, 2);
    std::cout << "  结果 = " << *raw << '\n';

    std::cout << "C++20 if constexpr：\n";
    auto vit2 = readings.begin();
    advance_by20(vit2, 4);
    std::cout << "  结果 = " << *vit2 << '\n';
    auto lit2 = pending.begin();
    advance_by20(lit2, 1);
    std::cout << "  结果 = " << *lit2 << '\n';

    std::cout << "traits 的 value_type：vector 和 list 都能传给 sum_all："
              << sum_all(readings.begin(), readings.end()) << " / "
              << sum_all(pending.begin(), pending.end()) << '\n';

    int raw_values[] = {7, 8, 9};
    std::cout << "裸指针 sum_all：" << sum_all(raw_values, raw_values + 3) << '\n';
}
