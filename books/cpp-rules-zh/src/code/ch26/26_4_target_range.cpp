// 第 26.4 节：把 end() 当目标区间起点，算法会写到不存在的元素上
// mode: crash
// expect: runtime error
#include <algorithm>
#include <cstdio>
#include <vector>

int main() {
    std::vector<int> prices{100, 250, 80};
    std::vector<int> withTax;  // 空容器，一个元素也没有

    std::puts("把 withTax.end() 当目标区间：");
    std::transform(prices.begin(), prices.end(), withTax.end(),
                   [](int price) { return price * 2; });
    std::printf("withTax.size() = %zu\n", withTax.size());
}
