// latency.cu: 指针追逐。一个线程沿着随机排列的链一路读下去，每一步都要等上一步的结果，
// 所以每步的时间就是一次访存的延迟。改变链覆盖的内存范围，就能看到各级缓存。
//   nvcc -O2 -arch=native latency.cu -o latency && ./latency
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <random>
#include <vector>

#define CHECK(call)                                                        \
    do {                                                                   \
        cudaError_t err_ = (call);                                         \
        if (err_ != cudaSuccess) {                                         \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__,        \
                    cudaGetErrorName(err_), cudaGetErrorString(err_));     \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

constexpr int STRIDE = 32;             // 链上相邻两个节点至少隔 32 个 unsigned（128 字节）
constexpr int STEPS = 1 << 16;         // 至少走这么多步；链更长时整条链走一遍

__constant__ unsigned c_next[16384];   // constant memory 一共 64 KB

// ANCHOR: chase
__global__ void chase_global(const unsigned* next, unsigned* out, int steps) {
    unsigned j = 0;
    for (int s = 0; s < steps; ++s) j = next[j];   // 下一次读的地址取决于这一次读到的值
    *out = j;
}
// ANCHOR_END: chase

// ANCHOR: shared
__global__ void chase_shared(const unsigned* next, int n, unsigned* out, int steps) {
    extern __shared__ unsigned s_next[];   // 大小在启动时由第三个参数指定
    for (int i = 0; i < n; ++i) s_next[i] = next[i];
    unsigned j = 0;
    for (int s = 0; s < steps; ++s) j = s_next[j];
    *out = j;
}
// ANCHOR_END: shared

__global__ void chase_constant(unsigned* out, int steps) {
    unsigned j = 0;
    for (int s = 0; s < steps; ++s) j = c_next[j];
    *out = j;
}

// 在 n 个元素里，每隔 STRIDE 个取一个节点，把这些节点随机串成一个环。
std::vector<unsigned> make_chain(size_t n) {
    size_t nodes = n / STRIDE;
    std::vector<unsigned> order(nodes);
    std::iota(order.begin(), order.end(), 0u);
    std::shuffle(order.begin() + 1, order.end(), std::mt19937(7));
    std::vector<unsigned> next(n, 0);
    for (size_t k = 0; k < nodes; ++k)
        next[size_t(order[k]) * STRIDE] = order[(k + 1) % nodes] * STRIDE;
    return next;
}

int main() {
    unsigned* d_out;
    CHECK(cudaMalloc(&d_out, sizeof(unsigned)));
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));
    auto elapsed_ns_per_step = [&](int steps, auto launch) {
        launch();                       // 预热，同时把数据带进缓存
        CHECK(cudaGetLastError());
        CHECK(cudaEventRecord(start));
        launch();
        CHECK(cudaEventRecord(stop));
        CHECK(cudaEventSynchronize(stop));
        float ms;
        CHECK(cudaEventElapsedTime(&ms, start, stop));
        return ms * 1e6 / steps;
    };

    printf("范围          global 延迟\n");
    for (size_t kb = 4; kb <= 512 * 1024; kb *= 2) {
        size_t n = kb * 1024 / sizeof(unsigned);
        std::vector<unsigned> next = make_chain(n);
        unsigned* d_next;
        CHECK(cudaMalloc(&d_next, n * sizeof(unsigned)));
        CHECK(cudaMemcpy(d_next, next.data(), n * sizeof(unsigned), cudaMemcpyHostToDevice));
        int steps = std::max<int>(STEPS, int(n / STRIDE));
        double ns = elapsed_ns_per_step(steps, [&] { chase_global<<<1, 1>>>(d_next, d_out, steps); });
        if (kb < 1024) printf("%6zu KB  %8.1f ns\n", kb, ns);
        else printf("%6zu MB  %8.1f ns\n", kb / 1024, ns);
        CHECK(cudaFree(d_next));
    }

    // shared memory 和 constant memory：都用 16 KB 的链。
    size_t n = 16384;
    std::vector<unsigned> next = make_chain(n);
    unsigned* d_next;
    CHECK(cudaMalloc(&d_next, n * sizeof(unsigned)));
    CHECK(cudaMemcpy(d_next, next.data(), n * sizeof(unsigned), cudaMemcpyHostToDevice));
    CHECK(cudaMemcpyToSymbol(c_next, next.data(), n * sizeof(unsigned)));
    // ANCHOR: launch_shared
    // 动态 shared memory 超过 48 KB 时要先申请；这里 64 KB。
    CHECK(cudaFuncSetAttribute(chase_shared, cudaFuncAttributeMaxDynamicSharedMemorySize,
                               int(n * sizeof(unsigned))));
    double sh = elapsed_ns_per_step(STEPS, [&] {
        chase_shared<<<1, 1, n * sizeof(unsigned)>>>(d_next, int(n), d_out, STEPS);
    });
    // ANCHOR_END: launch_shared
    double sh_fill = elapsed_ns_per_step(STEPS, [&] {   // 只填充 shared memory、不追逐的时间，用来扣除
        chase_shared<<<1, 1, n * sizeof(unsigned)>>>(d_next, int(n), d_out, 0);
    });
    double co = elapsed_ns_per_step(STEPS, [&] { chase_constant<<<1, 1>>>(d_out, STEPS); });
    printf("\nshared memory    %6.1f ns\nconstant memory  %6.1f ns\n", sh - sh_fill, co);
    CHECK(cudaFree(d_next));
    CHECK(cudaFree(d_out));
}
