// 第 22.4 节：用能满足需求的最抽象操作，同一份代码服务 vector、list 和数组
#include <forward_list>
#include <iostream>
#include <iterator>
#include <list>
#include <ranges>
#include <vector>

// 只要求前向迭代器：!= 比较，++ 前进。
template <std::forward_iterator It>
long long sum(It first, It last) {
    long long total = 0;
    for (; first != last; ++first) total += *first;
    return total;
}

// 只要求“能遍历”：C++20 的 range 概念把要求写在接口上。
template <std::ranges::forward_range R>
long long sum_range(const R& r) {
    long long total = 0;
    for (const auto& value : r) total += value;
    return total;
}

// 只问“空不空”，不要求“确切大小”。
template <class C>
bool has_items(const C& c) {
    return !c.empty();
}

// 反面教材：只接受 vector，list 和数组都进不来。
long long sum_vector_only(const std::vector<int>& v) {
    long long total = 0;
    for (int value : v) total += value;
    return total;
}

int main() {
    std::vector<int> readings{10, 20, 30};
    std::list<int> pending{10, 20, 30};
    std::forward_list<int> stream{10, 20, 30};
    int raw[] = {10, 20, 30};

    std::cout << "sum(vector)       = " << sum(readings.begin(), readings.end()) << '\n';
    std::cout << "sum(list)         = " << sum(pending.begin(), pending.end()) << '\n';
    std::cout << "sum(forward_list) = " << sum(stream.begin(), stream.end()) << '\n';
    std::cout << "sum(数组)         = " << sum(std::begin(raw), std::end(raw)) << '\n';

    std::cout << "sum_range(vector) = " << sum_range(readings) << '\n';
    std::cout << "sum_range(list)   = " << sum_range(pending) << '\n';

    std::cout << "has_items：vector " << has_items(readings)
              << "，空 list " << has_items(std::list<int>{}) << '\n';

    std::cout << "sum_vector_only(vector) = " << sum_vector_only(readings) << '\n';
    // sum_vector_only(pending);  // 编译错误：list 不是 vector
}
