// priority.cu: GPU 正被一个大 kernel 占满时，提交一个小 kernel，它要多久才能做完？
// 小 kernel 放在普通 stream 和高优先级 stream 上各试一次。
//   nvcc -O2 -arch=native priority.cu -o priority && ./priority
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#define CHECK(call)                                                        \
    do {                                                                   \
        cudaError_t err_ = (call);                                         \
        if (err_ != cudaSuccess) {                                         \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__,        \
                    cudaGetErrorName(err_), cudaGetErrorString(err_));     \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

// 每个 block 计算 iters 次，大约几十微秒到几毫秒。
__global__ void work(float* out, int iters) {
    float a = threadIdx.x;
    for (int k = 0; k < iters; ++k) a = a * 0.9999f + 0.5f;
    out[blockIdx.x * blockDim.x + threadIdx.x] = a;
}

int main() {
    float *big_out, *small_out;
    CHECK(cudaMalloc(&big_out, 20000 * 256 * sizeof(float)));
    CHECK(cudaMalloc(&small_out, 26 * 256 * sizeof(float)));
    // ANCHOR: create
    int least, greatest;                         // 数值越小优先级越高
    CHECK(cudaDeviceGetStreamPriorityRange(&least, &greatest));
    cudaStream_t bulk, normal, urgent;
    CHECK(cudaStreamCreateWithPriority(&bulk, cudaStreamNonBlocking, least));
    CHECK(cudaStreamCreateWithPriority(&normal, cudaStreamNonBlocking, least));
    CHECK(cudaStreamCreateWithPriority(&urgent, cudaStreamNonBlocking, greatest));
    // ANCHOR_END: create
    printf("优先级范围：%d（最低）到 %d（最高）\n", least, greatest);
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));
    work<<<26 * 6, 256, 0, bulk>>>(big_out, 1000);   // 预热
    work<<<26, 256, 0, urgent>>>(small_out, 1000);
    CHECK(cudaDeviceSynchronize());

    CHECK(cudaEventRecord(start, normal));       // 参照：GPU 空闲时，小 kernel 单独跑多久
    work<<<26, 256, 0, normal>>>(small_out, 10000);
    CHECK(cudaEventRecord(stop, normal));
    CHECK(cudaEventSynchronize(stop));
    float alone;
    CHECK(cudaEventElapsedTime(&alone, start, stop));
    printf("小 kernel 单独运行：%7.2f ms\n", alone);

    for (cudaStream_t s : {normal, urgent}) {
        // 大 kernel：20000 个 block，每个约 0.5 ms，一共要跑七十毫秒左右。
        work<<<20000, 256, 0, bulk>>>(big_out, 100000);
        usleep(5000);                            // 等它跑起来，把所有 SM 都占满
        CHECK(cudaEventRecord(start, s));
        work<<<26, 256, 0, s>>>(small_out, 10000);
        CHECK(cudaEventRecord(stop, s));
        CHECK(cudaEventSynchronize(stop));
        float ms;
        CHECK(cudaEventElapsedTime(&ms, start, stop));
        printf("小 kernel 放在%s stream：从提交到完成 %7.2f ms\n", s == urgent ? "高优先级" : "普通", ms);
        CHECK(cudaDeviceSynchronize());
    }
    CHECK(cudaFree(big_out));
    CHECK(cudaFree(small_out));
}
