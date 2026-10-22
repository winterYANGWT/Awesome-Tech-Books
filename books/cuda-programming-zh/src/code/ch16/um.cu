// um.cu: managed 内存按需迁移的代价、prefetch 的效果、CPU/GPU 交替读，以及 HMM 下的 malloc 内存。
//   nvcc -O2 -arch=native um.cu -o um && ./um
#include <chrono>
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

constexpr size_t N = size_t(64) << 20;    // 64M 个 float，256 MB

// 读写：每个元素乘一个系数。
__global__ void scale(float* p, size_t n, float k) {
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x)
        p[i] *= k;
}

// 只读：求和，结果放在普通显存里。
__global__ void sum(const float* p, size_t n, float* out) {
    float s = 0;
    for (size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x; i < n; i += size_t(gridDim.x) * blockDim.x)
        s += p[i];
    atomicAdd(out, s);
}

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

float* d_out;

double gpu_scale(float* p) {             // GPU 读写一遍，返回毫秒数
    double t0 = now_ms();
    scale<<<26 * 6, 256>>>(p, N, 1.0001f);
    CHECK(cudaGetLastError());
    CHECK(cudaDeviceSynchronize());
    return now_ms() - t0;
}

double gpu_sum(const float* p) {         // GPU 只读一遍
    double t0 = now_ms();
    sum<<<26 * 6, 256>>>(p, N, d_out);
    CHECK(cudaGetLastError());
    CHECK(cudaDeviceSynchronize());
    return now_ms() - t0;
}

volatile double sink;
double cpu_sum(const float* p) {         // CPU 只读一遍
    double t0 = now_ms(), s = 0;
    for (size_t i = 0; i < N; ++i) s += p[i];
    sink = s;
    return now_ms() - t0;
}

int main() {
    const int dev = 0;
    CHECK(cudaSetDevice(dev));
    CHECK(cudaMalloc(&d_out, sizeof(float)));
    cudaMemLocation gpu = {};
    gpu.type = cudaMemLocationTypeDevice;
    gpu.id = dev;

    // 参照：普通的 malloc 内存，CPU 读一遍要多久。
    float* plain = static_cast<float*>(malloc(N * sizeof(float)));
    for (size_t i = 0; i < N; ++i) plain[i] = 1.0f;
    printf("参照：CPU 读一遍普通内存 %6.1f ms\n\n", cpu_sum(plain));

    // ANCHOR: managed
    float* m;
    CHECK(cudaMallocManaged(&m, N * sizeof(float)));   // 主机和 GPU 用同一个指针
    for (size_t i = 0; i < N; ++i) m[i] = 1.0f;         // 在 CPU 上初始化，页面此时在主机内存里
    // ANCHOR_END: managed
    printf("managed 内存，CPU 初始化之后：\n");
    printf("  GPU 第一次读写     %7.1f ms\n", gpu_scale(m));
    printf("  GPU 第二次读写     %7.1f ms\n", gpu_scale(m));
    printf("  CPU 读一遍         %7.1f ms\n", cpu_sum(m));
    printf("  CPU 再读一遍       %7.1f ms\n", cpu_sum(m));
    // ANCHOR: prefetch
    double t0 = now_ms();
    CHECK(cudaMemPrefetchAsync(m, N * sizeof(float), gpu, 0, 0));   // 提前把整块搬到 GPU
    CHECK(cudaDeviceSynchronize());
    double t_pf = now_ms() - t0;
    // ANCHOR_END: prefetch
    double after = gpu_scale(m);
    printf("  prefetch 到 GPU    %7.1f ms，之后 GPU 读写 %5.1f ms\n\n", t_pf, after);

    // CPU 和 GPU 交替只读同一块数据 5 轮，比较加不加 ReadMostly。
    for (bool read_mostly : {false, true}) {
        if (read_mostly) {
            // ANCHOR: advise
            CHECK(cudaMemAdvise(m, N * sizeof(float), cudaMemAdviseSetReadMostly, gpu));
            // ANCHOR_END: advise
        }
        double g = 0, c = 0;
        for (int r = 0; r < 5; ++r) g += gpu_sum(m), c += cpu_sum(m);
        printf("交替读 5 轮%s：GPU 共 %6.1f ms，CPU 共 %6.1f ms\n",
               read_mostly ? "（ReadMostly）" : "            ", g, c);
    }
    CHECK(cudaFree(m));

    // ANCHOR: hmm
    // HMM：kernel 直接用 malloc 来的指针。
    printf("\nmalloc 内存（HMM），CPU 初始化之后：\n");
    printf("  GPU 第一次读写     %7.1f ms\n", gpu_scale(plain));
    printf("  GPU 第二次读写     %7.1f ms\n", gpu_scale(plain));
    printf("  CPU 读一遍         %7.1f ms\n", cpu_sum(plain));
    // ANCHOR_END: hmm
    free(plain);
    CHECK(cudaFree(d_out));
}
