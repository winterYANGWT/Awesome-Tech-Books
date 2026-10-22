// mathfn.cu: 同一批输入，GPU 和 CPU 各算一遍常用的单精度数学函数，和双精度的参考值比较误差（以 ulp 计），
// 再测 GPU 上每个函数的吞吐。分别用默认选项和 -use_fast_math 编译运行。
//   nvcc -O2 -arch=native mathfn.cu -o mathfn && ./mathfn
//   nvcc -O2 -arch=native -use_fast_math mathfn.cu -o mathfn_fast && ./mathfn_fast
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#define CHECK(call)                                                        \
    do {                                                                   \
        cudaError_t err_ = (call);                                         \
        if (err_ != cudaSuccess) {                                         \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__,        \
                    cudaGetErrorName(err_), cudaGetErrorString(err_));     \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

enum Fn { SIN, EXP, LOG, POW, SQRT, DIV, NFN };
const char* names[] = {"sinf(x)", "expf(x)", "logf(x)", "powf(x, 2.5f)", "sqrtf(x)", "1.0f / x"};
const float lo[] = {-10.0f, -80.0f, 1e-3f, 1e-2f, 0.0f, 0.1f}, hi[] = {10.0f, 80.0f, 1e3f, 1e2f, 1e6f, 1e3f};

// ANCHOR: apply
// 主机和设备共用的函数：设备上调用 CUDA 的数学库，主机上调用 C 库。
__host__ __device__ float apply(Fn f, float x) {
    switch (f) {
    case SIN: return sinf(x);
    case EXP: return expf(x);
    case LOG: return logf(x);
    case POW: return powf(x, 2.5f);
    case SQRT: return sqrtf(x);
    default: return 1.0f / x;
    }
}
double reference(Fn f, double x) {                    // 用双精度算的“准确值”
    switch (f) {
    case SIN: return sin(x);
    case EXP: return exp(x);
    case LOG: return log(x);
    case POW: return pow(x, 2.5);
    case SQRT: return sqrt(x);
    default: return 1.0 / x;
    }
}
// ANCHOR_END: apply

__global__ void eval(Fn f, const float* x, float* y, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) y[i] = apply(f, x[i]);
}
// 测吞吐：每个线程对 256 个不同的输入调用 f 并累加，计算量远大于访存。
__global__ void throughput(Fn f, const float* x, float* out, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    float base = x[i % n], s = 0;
    for (int r = 0; r < 256; ++r) s += apply(f, base + r * 1e-3f);
    out[i] = s;
}

double ulp_error(float y, double ref) {               // |y - ref| 是 ref 附近 float 间距的多少倍
    float r = float(ref);
    double ulp = fabs(double(nextafterf(r, INFINITY)) - double(r));
    if (ulp == 0 || !std::isfinite(ulp)) return 0;
    return fabs(double(y) - ref) / ulp;
}

int main() {
    const int n = 1 << 22;
    float *x, *y;
    CHECK(cudaMallocManaged(&x, n * sizeof(float)));
    CHECK(cudaMallocManaged(&y, n * sizeof(float)));
    float* out;
    CHECK(cudaMalloc(&out, 26 * 1536 * 8 * sizeof(float)));
    cudaEvent_t e0, e1;
    CHECK(cudaEventCreate(&e0));
    CHECK(cudaEventCreate(&e1));
    printf("函数            和 CPU 不同   GPU 最大误差（绝对误差）    CPU 最大误差      GPU 吞吐\n");
    for (int f = 0; f < NFN; ++f) {
        for (int i = 0; i < n; ++i) x[i] = lo[f] + (hi[f] - lo[f]) * (float(i) / n);   // 均匀取 n 个点
        eval<<<n / 256, 256>>>(Fn(f), x, y, n);
        CHECK(cudaDeviceSynchronize());
        long differ = 0;
        double gpu_err = 0, cpu_err = 0, gpu_abs = 0;
        for (int i = 0; i < n; ++i) {
            float c = apply(Fn(f), x[i]);
            double ref = reference(Fn(f), x[i]);
            differ += memcmp(&c, &y[i], sizeof c) != 0;
            gpu_err = fmax(gpu_err, ulp_error(y[i], ref));
            gpu_abs = fmax(gpu_abs, fabs(y[i] - ref));
            cpu_err = fmax(cpu_err, ulp_error(c, ref));
        }
        const int threads = 26 * 1536 * 8;
        throughput<<<threads / 256, 256>>>(Fn(f), x, out, n);
        CHECK(cudaEventRecord(e0));
        throughput<<<threads / 256, 256>>>(Fn(f), x, out, n);
        CHECK(cudaEventRecord(e1));
        CHECK(cudaEventSynchronize(e1));
        float ms;
        CHECK(cudaEventElapsedTime(&ms, e0, e1));
        printf("%-14s %9.2f%% %12.1f ulp（%7.1e） %9.1f ulp %8.0f 亿次/秒\n", names[f], 100.0 * differ / n, gpu_err,
               gpu_abs, cpu_err, threads * 256.0 / ms / 1e5);
    }
    // ANCHOR: denormal
    float tiny = 1e-39f;                              // 比最小的规格化 float（约 1.18e-38）还小，是次正规数
    eval<<<1, 1>>>(SQRT, &tiny, y, 1);                // 借用一下：sqrt(1e-39) 约 3.2e-20
    CHECK(cudaDeviceSynchronize());
    printf("次正规数：sqrtf(1e-39f) 在 GPU 上 = %g，CPU 上 = %g\n", y[0], sqrtf(tiny));
    // ANCHOR_END: denormal
}
