// fft.cu: cuFFT 的 plan 和执行各要多久；不同长度、批量变换、实数输入的速度。
//   nvcc -O2 -arch=native fft.cu -o fft -lcufft && ./fft
// NVCCFLAGS: -lcufft
#include <cufft.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
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

#define CHECK_FFT(call)                                                    \
    do {                                                                   \
        cufftResult r_ = (call);                                           \
        if (r_ != CUFFT_SUCCESS) {                                         \
            fprintf(stderr, "%s:%d: cuFFT 错误 %d\n", __FILE__, __LINE__,   \
                    int(r_));                                              \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

// 执行 f 若干次，返回每次的平均毫秒数。
template <class F>
double avg_ms(F f, int reps = 10) {
    f();
    CHECK(cudaDeviceSynchronize());
    double t0 = now_ms();
    for (int r = 0; r < reps; ++r) f();
    CHECK(cudaDeviceSynchronize());
    return (now_ms() - t0) / reps;
}

int main() {
    const size_t max_n = size_t(1) << 24;
    cufftComplex* x;
    CHECK(cudaMalloc(&x, (max_n + 16) * sizeof(cufftComplex)));
    std::vector<cufftComplex> h(max_n);
    for (size_t i = 0; i < max_n; ++i) h[i] = {std::sin(0.001f * i), std::cos(0.0007f * i)};

    // ANCHOR: plan
    const int n = 1 << 24;
    double t0 = now_ms();
    cufftHandle plan;
    CHECK_FFT(cufftPlan1d(&plan, n, CUFFT_C2C, 1));   // 决定算法、分配工作区
    double t_plan = now_ms() - t0;
    CHECK(cudaMemcpy(x, h.data(), n * sizeof(cufftComplex), cudaMemcpyHostToDevice));
    double t_exec = avg_ms([&] { CHECK_FFT(cufftExecC2C(plan, x, x, CUFFT_FORWARD)); });
    // ANCHOR_END: plan
    printf("长度 2^24 的复数 FFT：创建 plan %.1f ms，执行一次 %.2f ms\n", t_plan, t_exec);

    // ANCHOR: roundtrip
    // 验证：正变换再逆变换，除以 n 应该回到原始数据（cuFFT 的逆变换不做归一化）。
    CHECK(cudaMemcpy(x, h.data(), n * sizeof(cufftComplex), cudaMemcpyHostToDevice));
    CHECK_FFT(cufftExecC2C(plan, x, x, CUFFT_FORWARD));
    CHECK_FFT(cufftExecC2C(plan, x, x, CUFFT_INVERSE));
    std::vector<cufftComplex> back(n);
    CHECK(cudaMemcpy(back.data(), x, n * sizeof(cufftComplex), cudaMemcpyDeviceToHost));
    double worst = 0;
    for (int i = 0; i < n; i += 997)
        worst = std::fmax(worst, std::fabs(back[i].x / n - h[i].x) + std::fabs(back[i].y / n - h[i].y));
    // ANCHOR_END: roundtrip
    printf("正变换再逆变换，与原始数据的最大误差 %.1e\n\n", worst);
    CHECK_FFT(cufftDestroy(plan));

    // 不同的长度：2 的幂、只含小质因子、质数。
    printf("长度              创建 plan    执行一次\n");
    struct { int len; const char* desc; } sizes[] = {
        {1 << 24, "2^24"}, {16000000, "2^10·5^6"}, {16777213, "质数"}};
    for (auto& sz : sizes) {
        CHECK(cudaMemcpy(x, h.data(), sz.len * sizeof(cufftComplex), cudaMemcpyHostToDevice));
        double p0 = now_ms();
        cufftHandle p;
        CHECK_FFT(cufftPlan1d(&p, sz.len, CUFFT_C2C, 1));
        double tp = now_ms() - p0;
        double te = avg_ms([&] { CHECK_FFT(cufftExecC2C(p, x, x, CUFFT_FORWARD)); });
        printf("%9d（%s）%8.1f ms  %8.2f ms\n", sz.len, sz.desc, tp, te);
        CHECK_FFT(cufftDestroy(p));
    }

    // ANCHOR: batch
    // 同样 2^24 个点，分成 4096 个长度为 4096 的独立变换，一次调用做完。
    cufftHandle many;
    int len = 4096, batch = 4096;
    CHECK_FFT(cufftPlanMany(&many, 1, &len, nullptr, 1, len, nullptr, 1, len, CUFFT_C2C, batch));
    double tb = avg_ms([&] { CHECK_FFT(cufftExecC2C(many, x, x, CUFFT_FORWARD)); });
    // ANCHOR_END: batch
    printf("\n4096 个长度 4096 的变换（批量）：执行一次 %.2f ms\n", tb);
    CHECK_FFT(cufftDestroy(many));

    // ANCHOR: r2c
    // 实数输入：输出只需要一半多一点的复数（另一半是共轭对称的），计算量也减半。
    cufftHandle r2c;
    CHECK_FFT(cufftPlan1d(&r2c, n, CUFFT_R2C, 1));
    float* real_in = reinterpret_cast<float*>(x);              // 前 n 个 float 当作实数输入
    cufftComplex* spectrum = x + n / 2 + 8;                    // 输出放在后面，长度 n/2+1
    double tr = avg_ms([&] { CHECK_FFT(cufftExecR2C(r2c, real_in, spectrum)); });
    // ANCHOR_END: r2c
    printf("长度 2^24 的实数 FFT（R2C）：执行一次 %.2f ms\n", tr);
    CHECK_FFT(cufftDestroy(r2c));
    CHECK(cudaFree(x));
}
