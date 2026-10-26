// 第 26.2 节：带检查的 STL 在非法区间上当场断言，而不是等运行到崩溃
// mode: crash
// sanitize: none
// flags: -D_GLIBCXX_DEBUG
// expect: valid iterator range
#include <algorithm>
#include <cstdio>
#include <vector>

int main() {
    std::vector<int> v{1, 2, 3};
    std::puts("把 end() 当 first、begin() 当 last 传给算法：");
    std::for_each(v.end(), v.begin(), [](int) {});
}
