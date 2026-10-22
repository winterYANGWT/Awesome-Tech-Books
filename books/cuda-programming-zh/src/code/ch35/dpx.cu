// dpx.cu: DPX 内建函数（动态规划常用的“加了再取最大”）和手写的 max 比较速度。
// 在 sm_90 上它们编译成专门的指令；用 cuobjdump 看看在本机的架构上编译成了什么。
//   nvcc -O2 -arch=native dpx.cu -o dpx && ./dpx
//   nvcc -cubin -arch=sm_90 dpx.cu -o dpx90.cubin && cuobjdump -sass dpx90.cubin | grep -E "VIADDMNMX|VIMNMX"
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

// ANCHOR: kernels
// 每个线程沿一条“对角线”做 4096 步类似 Smith-Waterman 的递推：h = max(h + s, e, 0)，e = max(e - 1, h - 2)。
__global__ void dp_intrinsic(const int* s, int* out, int steps) {
    int h = 0, e = 0, idx = blockIdx.x * blockDim.x + threadIdx.x;
    for (int k = 0; k < steps; ++k) {
        int sc = s[(idx + k) & 1023];
        h = __viaddmax_s32_relu(h, sc, e);           // max(h + sc, e, 0)
        e = __viaddmax_s32(e, -1, h - 2);            // max(e - 1, h - 2)
    }
    out[idx] = h + e;
}
__global__ void dp_plain(const int* s, int* out, int steps) {
    int h = 0, e = 0, idx = blockIdx.x * blockDim.x + threadIdx.x;
    for (int k = 0; k < steps; ++k) {
        int sc = s[(idx + k) & 1023];
        h = max(max(h + sc, e), 0);
        e = max(e - 1, h - 2);
    }
    out[idx] = h + e;
}
// ANCHOR_END: kernels

int main() {
    const int threads = 26 * 1536 * 4, steps = 4096;
    int *s, *o1, *o2;
    CHECK(cudaMallocManaged(&s, 1024 * sizeof(int)));
    CHECK(cudaMalloc(&o1, threads * sizeof(int)));
    CHECK(cudaMalloc(&o2, threads * sizeof(int)));
    for (int i = 0; i < 1024; ++i) s[i] = (i * 37 % 11) - 5;   // -5..5 的打分
    cudaEvent_t e0, e1;
    CHECK(cudaEventCreate(&e0));
    CHECK(cudaEventCreate(&e1));
    float ms[2];
    for (int v = 0; v < 2; ++v) {
        auto run = [&] {
            if (v == 0) dp_intrinsic<<<threads / 256, 256>>>(s, o1, steps);
            else dp_plain<<<threads / 256, 256>>>(s, o2, steps);
        };
        run();
        CHECK(cudaEventRecord(e0));
        for (int r = 0; r < 10; ++r) run();
        CHECK(cudaEventRecord(e1));
        CHECK(cudaEventSynchronize(e1));
        CHECK(cudaEventElapsedTime(&ms[v], e0, e1));
    }
    int *h1 = new int[threads], *h2 = new int[threads], diff = 0;
    CHECK(cudaMemcpy(h1, o1, threads * sizeof(int), cudaMemcpyDeviceToHost));
    CHECK(cudaMemcpy(h2, o2, threads * sizeof(int), cudaMemcpyDeviceToHost));
    for (int i = 0; i < threads; ++i) diff += h1[i] != h2[i];
    printf("%d 个线程，每个 %d 步：DPX 内建函数 %.3f ms，手写 max %.3f ms，结果不同的线程 %d 个\n", threads, steps,
           ms[0] / 10, ms[1] / 10, diff);
}
