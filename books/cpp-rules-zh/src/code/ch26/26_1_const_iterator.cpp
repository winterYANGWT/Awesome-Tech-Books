// 第 26.1 节：const_iterator 的取得、插入、删除，以及它不能转回 iterator
#include <algorithm>
#include <array>
#include <cstdio>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

template <typename Container, typename T>
auto findFirst(const Container& c, const T& target) {
    // 非成员的 cbegin/cend 对容器和内建数组都成立（C++14 补齐）
    return std::find(std::cbegin(c), std::cend(c), target);
}

template <typename Container>
void print(const char* label, const Container& c) {
    std::printf("%s:", label);
    for (const auto& x : c) std::printf(" %d", x);
    std::puts("");
}

int main() {
    using Iter = std::vector<int>::iterator;
    using CIter = std::vector<int>::const_iterator;
    static_assert(std::is_convertible_v<Iter, CIter>);
    static_assert(!std::is_convertible_v<CIter, Iter>);  // 没有反向转换

    std::vector<int> temps{18, 21, 35, 22, 35};
    static_assert(std::is_same_v<decltype(std::as_const(temps).begin()), CIter>);

    auto firstHot = std::find(temps.cbegin(), temps.cend(), 35);
    std::printf("第一个 35 的下标：%ld\n",
                static_cast<long>(std::distance(temps.cbegin(), firstHot)));

    // 在 const_iterator 指出的位置前插入；不需要先转成 iterator
    temps.insert(firstHot, 0);
    print("插入后", temps);

    auto cold = std::find(temps.cbegin(), temps.cend(), 21);
    temps.erase(cold);  // erase 也接受 const_iterator
    print("删除 21 后", temps);

    std::array<int, 4> samples{4, 5, 6, 7};
    auto five = findFirst(samples, 5);
    std::printf("数组里 5 的下标：%ld\n",
                static_cast<long>(std::distance(samples.cbegin(), five)));
}
