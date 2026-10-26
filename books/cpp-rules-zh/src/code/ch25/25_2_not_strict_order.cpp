// 第 25.1 节：比较器对相等的值返回 true，严格弱序被破坏，sort 的划分越界
// mode: crash
// expect: heap-buffer-overflow
#include <algorithm>
#include <cstdio>
#include <vector>

int main() {
    // 20 个相同的等待时间：合格比较器对它们必须一律回答“不排在前面”
    std::vector<int> wait_minutes(20, 30);
    std::puts("开始排序");
    std::sort(wait_minutes.begin(), wait_minutes.end(),
              [](int a, int b) { return a <= b; });  // 错：相等时也返回 true
    std::puts("排序完成");
}
