// limits.cu: 寄存器用量怎样限制每个 SM 能驻留的 warp 数；grid 比一整轮多一个 block 时会怎样。
//   nvcc -O2 -arch=native limits.cu -o limits && ./limits
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

// R 个累加器互相依赖，整个循环里都活着，寄存器用量随 R 增长。
template <int R>
__global__ void live(float* out) {
    float acc[R];
#pragma unroll
    for (int j = 0; j < R; ++j) acc[j] = threadIdx.x + j;
    for (int k = 0; k < 64; ++k) {
#pragma unroll
        for (int j = 0; j < R; ++j) acc[j] = acc[j] * 0.999f + acc[(j + 1) % R] * 0.001f;
    }
    float s = 0;
#pragma unroll
    for (int j = 0; j < R; ++j) s += acc[j];
    out[blockIdx.x * blockDim.x + threadIdx.x] = s;
}

template <int R>
void report() {
    // ANCHOR: query
    cudaFuncAttributes attr;
    CHECK(cudaFuncGetAttributes(&attr, live<R>));          // 编译出来用了多少寄存器
    int blocks;
    CHECK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&blocks, live<R>, 256, 0));
    // ANCHOR_END: query
    printf("%3d 个寄存器  每 SM %d 个 block  %2d/48 个 warp\n", attr.numRegs, blocks, blocks * 8);
}

// ANCHOR: work
// 每个 block 做固定的一份计算，大约要几毫秒。
__global__ void work(float* out) {
    float a = threadIdx.x, b = a + 1, c = a + 2, d = a + 3;
    for (int k = 0; k < 200000; ++k) {
        a = a * 0.9999f + 0.5f;
        b = b * 0.9999f + 0.5f;
        c = c * 0.9999f + 0.5f;
        d = d * 0.9999f + 0.5f;
    }
    out[blockIdx.x * blockDim.x + threadIdx.x] = a + b + c + d;
}
// ANCHOR_END: work

int main() {
    printf("256 线程的 block：\n");
    report<8>();
    report<32>();
    report<48>();
    report<64>();
    report<96>();

    cudaDeviceProp prop;
    CHECK(cudaGetDeviceProperties(&prop, 0));
    float* out;
    CHECK(cudaMalloc(&out, size_t(prop.multiProcessorCount) * 6 * 11 * 256 * sizeof(float)));
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));
    for (int threads : {256, 1024}) {
        int per_sm;
        CHECK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&per_sm, work, threads, 0));
        int wave = prop.multiProcessorCount * per_sm;       // 一轮能同时跑的 block 数
        printf("\n%d 线程的 block，一轮最多同时运行 %d × %d = %d 个\n", threads,
               prop.multiProcessorCount, per_sm, wave);
        work<<<wave, threads>>>(out);                       // 预热
        for (int blocks : {wave, wave + 1, 2 * wave, 2 * wave + 1}) {
            CHECK(cudaEventRecord(start));
            work<<<blocks, threads>>>(out);
            CHECK(cudaEventRecord(stop));
            CHECK(cudaEventSynchronize(stop));
            float ms;
            CHECK(cudaEventElapsedTime(&ms, start, stop));
            printf("%5d 个 block  %7.2f ms\n", blocks, ms);
        }
    }
    CHECK(cudaFree(out));
}
