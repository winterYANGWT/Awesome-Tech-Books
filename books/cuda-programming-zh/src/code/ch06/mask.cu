// mask.cu: 在分支里打印 warp 的活跃线程掩码，看看硬件实际怎么执行 if/else。
//   nvcc -O2 -arch=native mask.cu -o mask && ./mask
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

// ANCHOR: kernel
// __activemask() 返回一个 32 位整数，第 k 位为 1 表示 lane k 此刻正在执行这条指令。
__global__ void show(const int* data) {
    int lane = threadIdx.x % 32;
    unsigned all = __activemask();
    if (data[lane] > 0) {
        unsigned m = __activemask();
        if (lane == __ffs(m) - 1)                // 由这一组里编号最小的线程打印
            printf("if   分支：%08x\n", m);
    } else {
        unsigned m = __activemask();
        if (lane == __ffs(m) - 1)
            printf("else 分支：%08x\n", m);
    }
    if (lane == 0) printf("分支之前：%08x\n", all);
}
// ANCHOR_END: kernel

int main() {
    int h[32];
    for (int i = 0; i < 32; ++i) h[i] = (i % 3 == 0) ? 1 : -1;   // lane 0, 3, 6, … 走 if
    int* d;
    CHECK(cudaMalloc(&d, sizeof(h)));
    CHECK(cudaMemcpy(d, h, sizeof(h), cudaMemcpyHostToDevice));
    show<<<1, 32>>>(d);
    CHECK(cudaGetLastError());
    CHECK(cudaDeviceSynchronize());
    CHECK(cudaFree(d));
}
