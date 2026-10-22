// alloc.cu: 每轮都要一块临时缓冲区。比较 cudaMalloc/cudaFree、cudaMallocAsync/cudaFreeAsync，
// 以及内存池释放阈值的影响。
//   nvcc -O2 -arch=native alloc.cu -o alloc && ./alloc
#include <chrono>
#include <cstdint>
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

constexpr size_t BYTES = size_t(64) << 20;   // 每轮一块 64 MB 的临时缓冲区
constexpr int ROUNDS = 200;

// 用临时缓冲区做一点事：写一遍再读一遍。
__global__ void use(float* tmp, float* out, size_t n) {
    float s = 0;
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x) {
        tmp[i] = float(i);
        s += tmp[i];
    }
    if (s == -1.0f) *out = s;                    // 几乎不会成立，只是为了让计算不被优化掉
}

double now_us() {
    using namespace std::chrono;
    return duration<double, std::micro>(steady_clock::now().time_since_epoch()).count();
}

int main() {
    const size_t n = BYTES / sizeof(float);
    float* out;
    CHECK(cudaMalloc(&out, sizeof(float)));
    cudaStream_t s;
    CHECK(cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking));
    cudaMemPool_t pool;
    CHECK(cudaDeviceGetDefaultMemPool(&pool, 0));
    const int grid = 26 * 6, block = 256;

    auto per_round = [&](const char* label, auto body) {
        body();                                  // 预热
        CHECK(cudaDeviceSynchronize());
        double t0 = now_us();
        for (int r = 0; r < ROUNDS; ++r) body();
        CHECK(cudaDeviceSynchronize());
        printf("%7.1f us  %s\n", (now_us() - t0) / ROUNDS, label);
    };

    // 参照：缓冲区在循环外分配一次。
    float* fixed;
    CHECK(cudaMalloc(&fixed, BYTES));
    per_round("参照：只分配一次", [&] { use<<<grid, block, 0, s>>>(fixed, out, n); });
    CHECK(cudaFree(fixed));

    // ANCHOR: sync_alloc
    per_round("cudaMalloc + cudaFree", [&] {
        float* tmp;
        CHECK(cudaMalloc(&tmp, BYTES));
        use<<<grid, block, 0, s>>>(tmp, out, n);
        CHECK(cudaFree(tmp));                    // 要等 GPU 上的工作做完才能释放
    });
    // ANCHOR_END: sync_alloc

    // ANCHOR: async_alloc
    per_round("cudaMallocAsync + cudaFreeAsync", [&] {
        float* tmp;
        CHECK(cudaMallocAsync(&tmp, BYTES, s));  // 分配和释放都排进 stream
        use<<<grid, block, 0, s>>>(tmp, out, n);
        CHECK(cudaFreeAsync(tmp, s));
    });
    // ANCHOR_END: async_alloc

    // 每轮结束都同步一次（比如要把结果拷回主机看）。
    per_round("同上，每轮同步，阈值为 0（默认）", [&] {
        float* tmp;
        CHECK(cudaMallocAsync(&tmp, BYTES, s));
        use<<<grid, block, 0, s>>>(tmp, out, n);
        CHECK(cudaFreeAsync(tmp, s));
        CHECK(cudaStreamSynchronize(s));
    });

    // ANCHOR: threshold
    // 让内存池在同步时不把内存还给系统。
    uint64_t keep = UINT64_MAX;
    CHECK(cudaMemPoolSetAttribute(pool, cudaMemPoolAttrReleaseThreshold, &keep));
    // ANCHOR_END: threshold
    per_round("同上，每轮同步，阈值为无限大", [&] {
        float* tmp;
        CHECK(cudaMallocAsync(&tmp, BYTES, s));
        use<<<grid, block, 0, s>>>(tmp, out, n);
        CHECK(cudaFreeAsync(tmp, s));
        CHECK(cudaStreamSynchronize(s));
    });

    // ANCHOR: stats
    uint64_t reserved, used, high;
    CHECK(cudaMemPoolGetAttribute(pool, cudaMemPoolAttrReservedMemCurrent, &reserved));
    CHECK(cudaMemPoolGetAttribute(pool, cudaMemPoolAttrUsedMemCurrent, &used));
    CHECK(cudaMemPoolGetAttribute(pool, cudaMemPoolAttrReservedMemHigh, &high));
    printf("\n内存池：当前保留 %llu MB，正在使用 %llu MB，最多时保留 %llu MB\n",
           (unsigned long long)(reserved >> 20), (unsigned long long)(used >> 20),
           (unsigned long long)(high >> 20));
    CHECK(cudaMemPoolTrimTo(pool, 0));           // 把不用的内存还给系统
    CHECK(cudaMemPoolGetAttribute(pool, cudaMemPoolAttrReservedMemCurrent, &reserved));
    printf("cudaMemPoolTrimTo(pool, 0) 之后保留 %llu MB\n", (unsigned long long)(reserved >> 20));
    // ANCHOR_END: stats
    CHECK(cudaFree(out));
}
