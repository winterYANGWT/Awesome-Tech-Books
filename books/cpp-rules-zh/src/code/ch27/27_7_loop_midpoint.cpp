// 第 27.7 节：把循环不变式写进注释，用 != 锁定次数；二分中点先减后加
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <vector>

int sum_first_n(int n) {
    int sum = 0;
    // 不变式：每轮开始时，sum 是 0 到 i-1 的和，且 0 <= i <= n。
    for (int i = 0; i != n; ++i) sum += i;
    // 退出时 i == n，所以 sum 是 0 到 n-1 的和。
    return sum;
}

// 只在随机访问区间上工作；返回第一个不小于 value 的位置。
template <std::random_access_iterator It, class T>
It lower_bound_hand(It first, It last, const T& value) {
    // 不变式：答案在 [first, last] 内；每轮把候选长度至少缩小一半。
    while (first != last) {
        It mid = first + (last - first) / 2;  // 先减后加，避免 first + last 溢出
        if (*mid < value)
            first = mid + 1;
        else
            last = mid;
    }
    return first;
}

int main() {
    std::cout << "sum_first_n(10)=" << sum_first_n(10) << '\n';

    std::vector<int> sorted{1, 3, 5, 7, 7, 9};
    auto seven = lower_bound_hand(sorted.begin(), sorted.end(), 7);
    auto four = lower_bound_hand(sorted.begin(), sorted.end(), 4);
    std::cout << "lower_bound_hand(7) -> 下标 " << (seven - sorted.begin())
              << "，值 " << *seven << '\n';
    std::cout << "lower_bound_hand(4) -> 下标 " << (four - sorted.begin())
              << "，值 " << *four << '\n';

    int lo = 2'000'000'000;
    int hi = 2'000'000'001;
    std::cout << "std::midpoint(" << lo << ", " << hi << ")=" << std::midpoint(lo, hi) << '\n';

    int data[10]{};
    int* mid = std::midpoint(data, data + 9);  // 指针也适用，要求指向同一数组
    std::cout << "指针中点相对偏移=" << (mid - data) << '\n';
}
