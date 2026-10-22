// connections.cu: 16 个 stream，每个 stream 里先后两个 kernel（后一个依赖前一个），每个 kernel 只有一个 block、跑 5 ms。
// 理想情况下 16 个 stream 完全并行，总共 10 ms。用 CUDA_DEVICE_MAX_CONNECTIONS 改变硬件工作队列的数量看看。
//   nvcc -O2 -arch=native connections.cu -o connections
//   for c in 1 2 4 8 16 32; do CUDA_DEVICE_MAX_CONNECTIONS=$c ./connections; done
#include <chrono>
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

__global__ void busy(long long ns) {
    long long t0, t;
    asm volatile("mov.u64 %0, %%globaltimer;" : "=l"(t0));
    do asm volatile("mov.u64 %0, %%globaltimer;" : "=l"(t)); while (t - t0 < ns);
}

int main() {
    const int S = 16;
    const long long ns = 5 * 1000000LL;
    cudaStream_t s[S];
    for (cudaStream_t& x : s) CHECK(cudaStreamCreateWithFlags(&x, cudaStreamNonBlocking));
    busy<<<1, 1>>>(1000);                             // 预热
    CHECK(cudaDeviceSynchronize());
    // ANCHOR: issue
    auto run = [&](bool depth_first) {
        auto t0 = std::chrono::steady_clock::now();
        if (depth_first) {                            // 一个 stream 的两个 kernel 连着发
            for (int i = 0; i < S; ++i) busy<<<1, 1, 0, s[i]>>>(ns), busy<<<1, 1, 0, s[i]>>>(ns);
        } else {                                      // 先发所有 stream 的第一个，再发第二个
            for (int i = 0; i < S; ++i) busy<<<1, 1, 0, s[i]>>>(ns);
            for (int i = 0; i < S; ++i) busy<<<1, 1, 0, s[i]>>>(ns);
        }
        CHECK(cudaDeviceSynchronize());
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    };
    // ANCHOR_END: issue
    const char* c = getenv("CUDA_DEVICE_MAX_CONNECTIONS");
    double df = run(true), bf = run(false);
    printf("CUDA_DEVICE_MAX_CONNECTIONS=%-6s 按 stream 依次发：%6.1f ms   交错着发：%6.1f ms\n", c ? c : "未设置", df, bf);
}
