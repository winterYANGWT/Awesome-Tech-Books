// mode: run
// expect: 块划分:
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

constexpr int ROWS = 1000;
constexpr int P = 4;

// 三角形工作量：第 i 行有 i+1 次迭代，总工作量是 1+2+…+ROWS。
uint64_t row_work(int row) { return uint64_t(row) + 1; }

std::vector<uint64_t> block_counts() {
    std::vector<uint64_t> got(P);
    for (int t = 0; t < P; ++t) {
        int lo = ROWS * t / P, hi = ROWS * (t + 1) / P;
        for (int r = lo; r < hi; ++r) got[t] += row_work(r);
    }
    return got;
}

std::vector<uint64_t> cyclic_counts() {
    std::vector<uint64_t> got(P);
    for (int t = 0; t < P; ++t)
        for (int r = t; r < ROWS; r += P) got[t] += row_work(r);
    return got;
}

std::vector<uint64_t> block_cyclic_counts(int chunk) {
    std::vector<uint64_t> got(P);
    for (int t = 0; t < P; ++t) {
        for (int base = t * chunk; base < ROWS; base += P * chunk) {
            int hi = std::min(base + chunk, ROWS);
            for (int r = base; r < hi; ++r) got[t] += row_work(r);
        }
    }
    return got;
}

void print(const char* name, const std::vector<uint64_t>& v) {
    std::printf("%s: ", name);
    for (uint64_t x : v) std::printf("%llu ", (unsigned long long)x);
    auto [lo, hi] = std::minmax_element(v.begin(), v.end());
    std::printf(" 最大/最小=%.2f\n", double(*hi) / double(*lo));
}

int main() {
    print("块划分", block_counts());
    print("循环划分", cyclic_counts());
    print("块-循环(c=2)", block_cyclic_counts(2));
}
