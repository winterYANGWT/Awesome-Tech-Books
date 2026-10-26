// 第 26.5 节：泛型函数只要求最弱的迭代器类别；手写迭代器要补齐 traits
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <iterator>
#include <list>
#include <numeric>
#include <ranges>
#include <sstream>
#include <vector>

// 只用了 !=、*、++，所以 list、vector、istream_iterator 都能传进来
template <typename It, typename T>
long countLess(It first, It last, const T& limit) {
    long n = 0;
    for (; first != last; ++first) {
        if (*first < limit) ++n;
    }
    return n;
}

// 手写一个前向迭代器：除了操作符，还要给 iterator_traits 五个嵌套类型
class ReadingIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using pointer = const int*;
    using reference = const int&;

    ReadingIterator() = default;
    explicit ReadingIterator(const int* p) : p_(p) {}

    reference operator*() const { return *p_; }
    ReadingIterator& operator++() {
        ++p_;
        return *this;
    }
    ReadingIterator operator++(int) {
        ReadingIterator old = *this;
        ++p_;
        return old;
    }
    friend bool operator==(const ReadingIterator&, const ReadingIterator&) = default;

private:
    const int* p_ = nullptr;
};
static_assert(std::forward_iterator<ReadingIterator>);

int main() {
    static_assert(std::input_iterator<std::istream_iterator<int>>);
    static_assert(std::forward_iterator<std::list<int>::iterator>);
    static_assert(std::bidirectional_iterator<std::list<int>::iterator>);
    static_assert(std::random_access_iterator<std::vector<int>::iterator>);
    static_assert(!std::random_access_iterator<std::list<int>::iterator>);
    static_assert(std::ranges::random_access_range<std::vector<int>>);
    static_assert(!std::ranges::random_access_range<std::list<int>>);

    std::vector<int> temperatures{12, 19, 25, 8, 31};
    std::list<int> readings{12, 19, 25, 8, 31};
    std::printf("vector 中低于 20 的：%ld\n", countLess(temperatures.begin(), temperatures.end(), 20));
    std::printf("list 中低于 20 的：%ld\n", countLess(readings.begin(), readings.end(), 20));

    std::istringstream input("4 8 15 16 23 42");
    std::istream_iterator<int> first(input), last;
    std::printf("输入迭代器求和：%d\n", std::accumulate(first, last, 0));

    std::istringstream more("1 30 5 40");
    std::printf("输入中低于 20 的：%ld\n",
                countLess(std::istream_iterator<int>(more), std::istream_iterator<int>(), 20));

    // 半开区间：end 是哨兵，只比较、不解引用；空区间满足 begin == end
    std::vector<int> empty;
    std::printf("空区间 begin==end：%s\n", empty.begin() == empty.end() ? "是" : "否");
    auto missing = std::find(temperatures.begin(), temperatures.end(), 99);
    std::printf("find 没找到时返回 end：%s\n", missing == temperatures.end() ? "是" : "否");

    const std::array<int, 6> sequence{1, 3, 6, 10, 15, 21};
    ReadingIterator seqBegin(sequence.data());
    ReadingIterator seqEnd(sequence.data() + sequence.size());
    std::printf("手写迭代器的 distance：%td\n", std::distance(seqBegin, seqEnd));
    std::printf("手写迭代器求和：%d\n", std::accumulate(seqBegin, seqEnd, 0));
}
