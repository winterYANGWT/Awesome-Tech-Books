// bugs.cu: 几种典型的 kernel 错误，每种用一个参数选择。
//   nvcc -O2 -lineinfo -arch=native bugs.cu -o bugs
// NVCCFLAGS: -lineinfo
//   ./bugs edge | near | far | race | uninit | assert
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define CHECK(call)                                                        \
    do {                                                                   \
        cudaError_t err_ = (call);                                         \
        if (err_ != cudaSuccess) {                                         \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__,        \
                    cudaGetErrorName(err_), cudaGetErrorString(err_));     \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

// ANCHOR: edge
// 漏了边界检查：最后一个 block 里多出来的线程会写到数组外面。
__global__ void add_no_check(const float* a, const float* b, float* c) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    c[i] = a[i] + b[i];
}
// ANCHOR_END: edge

// ANCHOR: far
// 下标算错：本意是 p[i]，却乘上了 stride。
__global__ void scale_wrong(float* p, int n, int stride) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) p[i * stride] *= 2.0f;
}
// ANCHOR_END: far

// ANCHOR: race
// 每个 block 把自己那一段数组反转。先写进 shared memory，再按反序读出来，
// 但两步之间忘了 __syncthreads()：读的时候，别的线程可能还没写。
__global__ void reverse_block(float* p) {
    __shared__ float tile[256];
    int t = threadIdx.x;
    int base = blockIdx.x * blockDim.x;
    tile[t] = p[base + t];
    // __syncthreads();
    p[base + t] = tile[blockDim.x - 1 - t];
}
// ANCHOR_END: race

// ANCHOR: uninit
// 读了一块分配了但从没写过的显存。
__global__ void sum_first(const float* p, float* out) {
    *out = p[0] + p[1];
}
// ANCHOR_END: uninit

// ANCHOR: assert
// kernel 里也可以用 assert 检查自己的假设：输入应该都是非负数。
__global__ void take_sqrt(float* p, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    assert(p[i] >= 0.0f);
    p[i] = sqrtf(p[i]);
}
// ANCHOR_END: assert

int main(int argc, char** argv) {
    const char* what = argc > 1 ? argv[1] : "";
    const int n = 1000;                  // 不是 256 的倍数
    float *a, *b, *c;
    CHECK(cudaMalloc(&a, n * sizeof(float)));
    CHECK(cudaMalloc(&b, n * sizeof(float)));
    CHECK(cudaMalloc(&c, n * sizeof(float)));
    CHECK(cudaMemset(a, 0, n * sizeof(float)));
    CHECK(cudaMemset(b, 0, n * sizeof(float)));
    int blocks = (n + 255) / 256;

    if (!strcmp(what, "edge")) {
        add_no_check<<<blocks, 256>>>(a, b, c);
    } else if (!strcmp(what, "near")) {
        scale_wrong<<<blocks, 256>>>(a, n, 1024);      // 最远越界 4 MB
    } else if (!strcmp(what, "far")) {
        scale_wrong<<<blocks, 256>>>(a, n, 1000000);   // 最远越界 4 GB
    } else if (!strcmp(what, "race")) {
        float *p, h[1024];
        for (int i = 0; i < 1024; ++i) h[i] = float(i);
        CHECK(cudaMalloc(&p, sizeof(h)));
        CHECK(cudaMemcpy(p, h, sizeof(h), cudaMemcpyHostToDevice));
        reverse_block<<<4, 256>>>(p);
        CHECK(cudaMemcpy(h, p, sizeof(h), cudaMemcpyDeviceToHost));
        int wrong = 0;
        for (int i = 0; i < 1024; ++i) wrong += h[i] != float((i / 256) * 256 + 255 - i % 256);
        printf("%d 个元素反转错了\n", wrong);
    } else if (!strcmp(what, "uninit")) {
        float* fresh;
        CHECK(cudaMalloc(&fresh, 2 * sizeof(float)));   // 没有初始化
        sum_first<<<1, 1>>>(fresh, c);
    } else if (!strcmp(what, "assert")) {
        float minus = -1.0f;
        CHECK(cudaMemcpy(a + 500, &minus, sizeof(float), cudaMemcpyHostToDevice));
        take_sqrt<<<blocks, 256>>>(a, n);              // a[500] 是负数
    } else {
        fprintf(stderr, "usage: %s edge | near | far | race | uninit | assert\n", argv[0]);
        return 2;
    }

    // ANCHOR: after
    // 不用 CHECK 退出，把每一步的结果打印出来，看看出错以后会怎样。
    cudaError_t launch = cudaGetLastError();
    cudaError_t sync = cudaDeviceSynchronize();
    printf("启动：%s，同步：%s\n", cudaGetErrorName(launch), cudaGetErrorName(sync));
    float* more;
    cudaError_t alloc = cudaMalloc(&more, 1024);      // 一次和 kernel 毫无关系的分配
    printf("之后的 cudaMalloc：%s\n", cudaGetErrorName(alloc));
    // ANCHOR_END: after
    return 0;
}
