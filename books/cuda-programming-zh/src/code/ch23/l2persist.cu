// l2persist.cu: 一张 16 MB 的查找表被随机地反复访问，同时有 256 MB 的数据流过 L2。
// 比较不做设置、以及把查找表设为 L2 持久化窗口两种情况。
//   nvcc -O2 -arch=native l2persist.cu -o l2persist && ./l2persist
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

size_t TABLE = size_t(4) << 20;                // 查找表的元素个数，默认 4M 个 float（16 MB），可由命令行指定 MB 数
constexpr size_t STREAM = size_t(64) << 20;    // 64M 个 float，256 MB

// ANCHOR: kernel
// 每个元素顺序地读、写一次（流式数据），同时随机查一次表（反复使用的数据）。
__global__ void lookup(const float* in, float* out, const float* table, size_t table_n, size_t n) {
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x) {
        unsigned h = unsigned(i) * 2654435761u;    // 简单的哈希，让查表的位置看起来是随机的
        out[i] = in[i] + table[h % table_n];
    }
}
// ANCHOR_END: kernel

int main(int argc, char** argv) {
    if (argc > 1) TABLE = size_t(atoi(argv[1])) << 18;   // MB 数 × 2^18 个 float
    float *in, *out, *table;
    CHECK(cudaMalloc(&in, STREAM * sizeof(float)));
    CHECK(cudaMalloc(&out, STREAM * sizeof(float)));
    CHECK(cudaMalloc(&table, TABLE * sizeof(float)));
    CHECK(cudaMemset(in, 0, STREAM * sizeof(float)));
    CHECK(cudaMemset(table, 0, TABLE * sizeof(float)));
    cudaStream_t s;
    CHECK(cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking));
    cudaDeviceProp prop;
    CHECK(cudaGetDeviceProperties(&prop, 0));
    printf("查找表 %zu MB；L2 %d MB，最多可留给持久化数据 %d MB\n", TABLE * sizeof(float) >> 20,
           prop.l2CacheSize >> 20, prop.persistingL2CacheMaxSize >> 20);
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));

    auto run = [&](const char* label) {
        float best = 1e30f;
        for (int r = 0; r < 5; ++r) {
            CHECK(cudaEventRecord(start, s));
            lookup<<<26 * 6, 256, 0, s>>>(in, out, table, TABLE, STREAM);
            CHECK(cudaEventRecord(stop, s));
            CHECK(cudaEventSynchronize(stop));
            CHECK(cudaGetLastError());
            float ms;
            CHECK(cudaEventElapsedTime(&ms, start, stop));
            if (r > 0 && ms < best) best = ms;
        }
        printf("%7.2f ms  %s\n", best, label);
    };
    run("不做设置");

    // ANCHOR: window
    // 先从 L2 里划出一块给持久化数据，再告诉这条 stream：查找表这段地址的访问要尽量留在那里。
    CHECK(cudaDeviceSetLimit(cudaLimitPersistingL2CacheSize, prop.persistingL2CacheMaxSize));
    cudaStreamAttrValue attr = {};
    attr.accessPolicyWindow.base_ptr = table;
    attr.accessPolicyWindow.num_bytes = TABLE * sizeof(float);
    // 窗口比持久化区域大时，只让其中一部分（按比例）的访问按持久化处理，免得互相挤占。
    double fit = double(prop.persistingL2CacheMaxSize) / (TABLE * sizeof(float));
    attr.accessPolicyWindow.hitRatio = float(fit < 1.0 ? fit : 1.0);
    attr.accessPolicyWindow.hitProp = cudaAccessPropertyPersisting;
    attr.accessPolicyWindow.missProp = cudaAccessPropertyStreaming;
    CHECK(cudaStreamSetAttribute(s, cudaStreamAttributeAccessPolicyWindow, &attr));
    // ANCHOR_END: window
    run("查找表设为持久化窗口");

    // 用完要恢复：清掉窗口，把已经标成持久化的缓存行重置回普通状态，再把限额设回 0。
    attr.accessPolicyWindow.num_bytes = 0;
    CHECK(cudaStreamSetAttribute(s, cudaStreamAttributeAccessPolicyWindow, &attr));
    CHECK(cudaCtxResetPersistingL2Cache());
    run("清掉窗口，但 20 MB 限额还在");
    CHECK(cudaDeviceSetLimit(cudaLimitPersistingL2CacheSize, 0));
    run("限额也设回 0");
    CHECK(cudaFree(in));
    CHECK(cudaFree(out));
    CHECK(cudaFree(table));
}
