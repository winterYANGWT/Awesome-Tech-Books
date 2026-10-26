// 第 27.6 节：copy_if 可以自己写，但不要给谓词加额外的可适配要求；索引表用迭代器接口
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

// 正确的最小实现：不要求谓词可适配，返回值是更新后的输出迭代器。
template <class In, class Out, class Pred>
Out copy_if_impl(In first, In last, Out out, Pred pred) {
    while (first != last) {
        if (pred(*first)) *out++ = *first;
        ++first;
    }
    return out;
}

// 索引表：不改原序列，只排序“下标”。输入输出都用迭代器，调用方决定写到哪。
template <class RandomIt, class Out>
Out index_table(RandomIt first, RandomIt last, Out out) {
    using Diff = typename std::iterator_traits<RandomIt>::difference_type;
    std::vector<Diff> idx(static_cast<std::size_t>(last - first));
    std::iota(idx.begin(), idx.end(), Diff{0});

    std::sort(idx.begin(), idx.end(), [&](Diff a, Diff b) {
        if (*(first + a) != *(first + b)) return *(first + a) > *(first + b);
        return a < b;  // 同分按原下标，保证输出确定
    });

    return std::transform(idx.begin(), idx.end(), out, [](Diff i) { return i; });
}

int main() {
    std::vector<std::string> names{"Ada", "Barbara", "Cyd", "Deborah", "Eve"};
    auto long_enough = [](const std::string& s) { return s.size() >= 5; };

    std::vector<std::string> with_std;
    std::copy_if(names.begin(), names.end(), std::back_inserter(with_std), long_enough);

    std::vector<std::string> with_own;
    copy_if_impl(names.begin(), names.end(), std::back_inserter(with_own), long_enough);

    std::cout << "std::copy_if：";
    for (const auto& s : with_std) std::cout << s << ' ';
    std::cout << "\n自己写的 copy_if：";
    for (const auto& s : with_own) std::cout << s << ' ';
    std::cout << '\n';

    std::vector<int> scores{88, 95, 70, 95, 60, 88};
    std::vector<std::ptrdiff_t> table;
    index_table(scores.begin(), scores.end(), std::back_inserter(table));

    std::cout << "索引表（分数降序，同分按原下标）：";
    for (auto i : table) std::cout << i << ':' << scores[static_cast<std::size_t>(i)] << ' ';
    std::cout << "\n原数组未改：";
    for (int s : scores) std::cout << s << ' ';
    std::cout << '\n';
}
