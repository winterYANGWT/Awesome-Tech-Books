// tiny.cu: 一个最小的程序，用来观察 nvcc 的编译步骤和产物。
//   nvcc -O2 -arch=native tiny.cu -o tiny && ./tiny
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
__global__ void saxpy(int n, float a, const float* x, float* y) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) y[i] = a * x[i] + y[i];
}
// ANCHOR_END: kernel

int main() {
    const int n = 1 << 20;
    float *x, *y;
    CHECK(cudaMalloc(&x, n * sizeof(float)));
    CHECK(cudaMalloc(&y, n * sizeof(float)));
    saxpy<<<(n + 255) / 256, 256>>>(n, 2.0f, x, y);
    CHECK(cudaGetLastError());
    CHECK(cudaDeviceSynchronize());
    printf("saxpy 运行成功\n");
    CHECK(cudaFree(x));
    CHECK(cudaFree(y));
}
