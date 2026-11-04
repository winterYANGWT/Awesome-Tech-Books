// mode: run
// flags: -fopenmp
// tsan: no（libgomp 未插桩）
// expect: simd_sum=50000005000000 parallel_sum=50000005000000 equal=1

#include <cstdint>
#include <cstdio>
#include <omp.h>

int main() {
    constexpr std::int64_t n = 10'000'000;
    std::int64_t simd_sum = 0;
    std::int64_t parallel_sum = 0;

    // simd 不创建线程：同一条线程把多个迭代打包成向量指令。
#pragma omp simd reduction(+ : simd_sum)
    for (std::int64_t i = 1; i <= n; ++i) {
        simd_sum += i;
    }

#pragma omp parallel for reduction(+ : parallel_sum) schedule(static)
    for (std::int64_t i = 1; i <= n; ++i) {
        parallel_sum += i;
    }

    std::printf("simd_sum=%lld parallel_sum=%lld equal=%d\n",
                static_cast<long long>(simd_sum), static_cast<long long>(parallel_sum),
                simd_sum == parallel_sum ? 1 : 0);
    return simd_sum == parallel_sum ? 0 : 1;
}
