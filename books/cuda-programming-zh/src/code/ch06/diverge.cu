// diverge.cu: 同样的计算量，分支按 warp 划分和按线程划分，各要多久。
//   nvcc -O2 -arch=native diverge.cu -o diverge && ./diverge
#include <cstdio>
#include <cstdlib>

#define CHECK(call)                                                        \
    do {                                                                   \
        cudaError_t err_ = (call);                                         \
        if (err_ != cudaSuccess) {                                         \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__,        \
                    cudaGetErrorName(err_), cudaGetErrorString(err_));     \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

constexpr int ITERS = 20000;

// ANCHOR: kernel
// 每个线程属于 ways 条路径中的一条，只做自己那条路径的计算。
// by_warp 为真时，同一个 warp 的 32 个线程走同一条路径；否则相邻线程走不同的路径。
__global__ void branchy(float* out, int ways, bool by_warp) {
    int t = blockIdx.x * blockDim.x + threadIdx.x;
    int lane = threadIdx.x % 32, warp = t / 32;
    int path = by_warp ? warp % ways : lane % ways;
    float v = float(t);
    for (int p = 0; p < ways; ++p) {
        if (path == p) {
            for (int k = 0; k < ITERS; ++k) v = v * 0.9999f + float(p);
        }
    }
    out[t] = v;
}
// ANCHOR_END: kernel

int main() {
    const int blocks = 26 * 6 * 4, threads = 256;   // 每个 SM 若干个满载的 block
    float* out;
    CHECK(cudaMalloc(&out, blocks * threads * sizeof(float)));
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));

    auto time = [&](int ways, bool by_warp) {
        branchy<<<blocks, threads>>>(out, ways, by_warp);   // 预热
        CHECK(cudaGetLastError());
        float best = 1e30f;
        for (int r = 0; r < 5; ++r) {
            CHECK(cudaEventRecord(start));
            branchy<<<blocks, threads>>>(out, ways, by_warp);
            CHECK(cudaEventRecord(stop));
            CHECK(cudaEventSynchronize(stop));
            float ms;
            CHECK(cudaEventElapsedTime(&ms, start, stop));
            if (ms < best) best = ms;
        }
        return best;
    };

    printf("路径数   按 warp 分     按线程分\n");
    for (int ways : {1, 2, 4, 8, 16, 32}) {
        float w = time(ways, true);    // 分开调用：函数参数的求值顺序是不确定的
        float t = time(ways, false);
        printf("%4d    %7.2f ms   %8.2f ms\n", ways, w, t);
    }
    CHECK(cudaFree(out));
}
