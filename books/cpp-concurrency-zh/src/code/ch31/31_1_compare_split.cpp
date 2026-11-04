// compare-split 的局部版本：只保留自己那一半，不把 2n/p 个元素完整排序
#include <algorithm>
#include <cstdio>
#include <vector>

// 两个有序数组，取并集里最小的 need 个；比较次数用 comps 带回
static std::vector<int> merge_low(const std::vector<int>& a,
                                  const std::vector<int>& b, int need,
                                  int& comps) {
    std::vector<int> out;
    out.reserve(need);
    std::size_t i = 0, j = 0;
    comps = 0;
    while (static_cast<int>(out.size()) < need) {
        ++comps;
        if (j >= b.size() || (i < a.size() && a[i] <= b[j]))
            out.push_back(a[i++]);
        else
            out.push_back(b[j++]);
    }
    return out;
}

// 取并集里最大的 need 个：从后往前合并
static std::vector<int> merge_high(const std::vector<int>& a,
                                   const std::vector<int>& b, int need,
                                   int& comps) {
    std::vector<int> out(need);
    std::size_t i = a.size(), j = b.size();
    int k = need;
    comps = 0;
    while (k > 0) {
        ++comps;
        if (j == 0 || (i > 0 && a[i - 1] >= b[j - 1]))
            out[--k] = a[--i];
        else
            out[--k] = b[--j];
    }
    return out;
}

int main() {
    std::vector<int> lo{1, 4, 7, 10, 13, 16};
    std::vector<int> hi{2, 3, 8, 9, 14, 15};
    const int n = 6;

    int low_comps = 0, high_comps = 0;
    std::vector<int> low = merge_low(lo, hi, n, low_comps);
    std::vector<int> high = merge_high(lo, hi, n, high_comps);

    std::printf("rank 0 的块:");
    for (int x : lo) std::printf(" %d", x);
    std::printf("\nrank 1 的块:");
    for (int x : hi) std::printf(" %d", x);

    std::printf("\nrank 0 用 Merge_low 保留:");
    for (int x : low) std::printf(" %d", x);
    std::printf("（%d 个，比较 %d 次）\n", n, low_comps);

    std::printf("rank 1 用 Merge_high 保留:");
    for (int x : high) std::printf(" %d", x);
    std::printf("（%d 个，比较 %d 次）\n", n, high_comps);

    std::vector<int> whole = low;
    whole.insert(whole.end(), high.begin(), high.end());
    std::printf("拼接后:");
    for (int x : whole) std::printf(" %d", x);
    std::printf("\n全局有序: %s；只对 2n/p=%d 个元素各扫了一遍，没有完整排序\n",
                std::is_sorted(whole.begin(), whole.end()) ? "是" : "否", 2 * n);
    return 0;
}
