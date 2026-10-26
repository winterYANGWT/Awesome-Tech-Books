// 第 27.6 节：C++17 移除了 ptr_fun，C++20 又移除了 not1；libstdc++ 15 仍以弃用形式提供
// mode: warn
// expect: deprecated
#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <vector>

bool is_odd(int x) { return x % 2 != 0; }

int main() {
    std::vector<int> values{1, 2, 3, 4};
    std::remove_copy_if(values.begin(), values.end(),
                        std::ostream_iterator<int>(std::cout, " "),
                        std::not1(std::ptr_fun(is_odd)));
    std::cout << '\n';
}
