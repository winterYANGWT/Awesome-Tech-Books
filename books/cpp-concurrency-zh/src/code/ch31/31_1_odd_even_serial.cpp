// 串行奇偶交换排序：每个阶段只比较不相邻的一对，阶段内可以并行
#include <algorithm>
#include <cstdio>
#include <vector>

int main() {
    std::vector<int> a{9, 1, 8, 2, 7, 3, 6, 4, 5, 0};
    const int n = static_cast<int>(a.size());

    std::printf("初始:");
    for (int x : a) std::printf(" %d", x);
    std::printf("\n");

    int phases = 0;
    for (int phase = 0; phase < n; ++phase) {
        int swaps = 0;
        for (int i = phase % 2; i + 1 < n; i += 2) {
            if (a[i] > a[i + 1]) {
                std::swap(a[i], a[i + 1]);
                ++swaps;
            }
        }
        ++phases;
        std::printf("阶段 %d（%s）交换 %d 次:", phase,
                    phase % 2 == 0 ? "偶" : "奇", swaps);
        for (int x : a) std::printf(" %d", x);
        std::printf("\n");
        if (swaps == 0) break;  // 串行版可以提前退出；并行版不能各自退出
    }
    std::printf("总阶段数: %d（上限 n=%d）\n", phases, n);
    return 0;
}
