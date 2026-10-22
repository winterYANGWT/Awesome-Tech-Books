// overflow.cu: 数组超过 2^31 个元素时，用 int 算下标会溢出。
//   nvcc -O2 -arch=native overflow.cu -o overflow && ./overflow int && ./overflow size_t
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

// ANCHOR: kernels
__global__ void fill_int(unsigned char* p, size_t n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;              // 超过 2^31 就变成负数
    if (i < n) p[i] = 1;
}

__global__ void fill_size_t(unsigned char* p, size_t n) {
    size_t i = size_t(blockIdx.x) * blockDim.x + threadIdx.x;   // 先转成 64 位再乘
    if (i < n) p[i] = 1;
}
// ANCHOR_END: kernels

int main(int argc, char** argv) {
    bool use_int = argc > 1 && !strcmp(argv[1], "int");
    const size_t n = size_t(5) << 29;                // 25 亿多字节，超过 2^31
    unsigned char* d;
    CHECK(cudaMalloc(&d, n));
    CHECK(cudaMemset(d, 0, n));
    unsigned blocks = unsigned((n + 255) / 256);
    if (use_int)
        fill_int<<<blocks, 256>>>(d, n);
    else
        fill_size_t<<<blocks, 256>>>(d, n);
    CHECK(cudaGetLastError());
    CHECK(cudaDeviceSynchronize());
    unsigned char last;
    CHECK(cudaMemcpy(&last, d + n - 1, 1, cudaMemcpyDeviceToHost));
    printf("%s 下标：最后一个字节 = %d\n", use_int ? "int" : "size_t", last);
    CHECK(cudaFree(d));
}
