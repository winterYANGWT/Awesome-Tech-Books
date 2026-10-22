// rtc.cpp: 程序运行时生成 kernel 源码，用 NVRTC 编译，再用 Driver API 装载和启动。
// 这个文件里没有一行设备代码，用普通的 C++ 编译器也能编译。
//   g++ -O2 rtc.cpp -I/usr/local/cuda/include -L/usr/local/cuda/lib64 -lnvrtc -lcuda -o rtc && ./rtc
// NVCCFLAGS: -lnvrtc -lcuda
#include <cuda.h>
#include <nvrtc.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

#define CU(call)                                                                   \
    do {                                                                           \
        CUresult r_ = (call);                                                      \
        if (r_ != CUDA_SUCCESS) {                                                  \
            const char *name, *msg;                                                \
            cuGetErrorName(r_, &name), cuGetErrorString(r_, &msg);                 \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__, name, msg);    \
            exit(1);                                                               \
        }                                                                          \
    } while (0)
#define RTC(call)                                                                  \
    do {                                                                           \
        nvrtcResult r_ = (call);                                                   \
        if (r_ != NVRTC_SUCCESS) {                                                 \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, nvrtcGetErrorString(r_)); \
            exit(1);                                                               \
        }                                                                          \
    } while (0)

static double now_ms() {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// ANCHOR: compile
// 把一段源码编译成 PTX（arch 写 compute_XX）或 cubin（arch 写 sm_XX）。
std::string compile(const std::string& src, const char* arch, double* ms) {
    double t0 = now_ms();
    nvrtcProgram prog;
    RTC(nvrtcCreateProgram(&prog, src.c_str(), "poly.cu", 0, nullptr, nullptr));
    std::string opt = std::string("-arch=") + arch;
    const char* opts[] = {opt.c_str()};
    nvrtcResult r = nvrtcCompileProgram(prog, 1, opts);
    if (r != NVRTC_SUCCESS) {                         // 编译错误和 nvcc 一样有日志，要打出来
        size_t n;
        nvrtcGetProgramLogSize(prog, &n);
        std::string log(n, '\0');
        nvrtcGetProgramLog(prog, &log[0]);
        fprintf(stderr, "%s\n", log.c_str());
        exit(1);
    }
    size_t n;
    std::string out;
    if (arch[0] == 's') {
        RTC(nvrtcGetCUBINSize(prog, &n));
        out.resize(n);
        RTC(nvrtcGetCUBIN(prog, &out[0]));
    } else {
        RTC(nvrtcGetPTXSize(prog, &n));
        out.resize(n);
        RTC(nvrtcGetPTX(prog, &out[0]));
    }
    RTC(nvrtcDestroyProgram(&prog));
    *ms = now_ms() - t0;
    return out;
}
// ANCHOR_END: compile

// ANCHOR: source
// 通用版：多项式的次数和系数都是参数，系数每次从显存（缓存）里读。
const char* generic_src = R"(
extern "C" __global__ void poly(int n, int degree, const float* __restrict__ c,
                                const float* __restrict__ x, float* __restrict__ y) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    float v = x[i], r = c[0];
    for (int k = 1; k <= degree; ++k) r = r * v + c[k];
    y[i] = r;
}
)";

// 专用版：拿到具体的系数以后再生成源码，次数和系数都成了写死在代码里的常量。
std::string specialized_src(const std::vector<float>& c) {
    std::string s = "extern \"C\" __global__ void poly(int n, int, const float*, const float* __restrict__ x,\n"
                    "                                float* __restrict__ y) {\n"
                    "    int i = blockIdx.x * blockDim.x + threadIdx.x;\n"
                    "    if (i >= n) return;\n";
    char buf[64];
    snprintf(buf, sizeof buf, "    float v = x[i], r = %.9gf;\n", c[0]);
    s += buf;
    for (size_t k = 1; k < c.size(); ++k) {
        snprintf(buf, sizeof buf, "    r = r * v + %.9gf;\n", c[k]);   // %.9g 能精确还原一个 float
        s += buf;
    }
    return s + "    y[i] = r;\n}\n";
}
// ANCHOR_END: source

int main() {
    const int n = 1 << 24, degree = 512;
    std::mt19937 rng(1);
    std::uniform_real_distribution<float> u(-1, 1);
    std::vector<float> c(degree + 1), x(n), y(n), y_ref(n);
    for (float& v : c) v = 0.5f * u(rng);
    for (float& v : x) v = u(rng);

    // ANCHOR: driver
    CU(cuInit(0));
    CUdevice dev;
    CU(cuDeviceGet(&dev, 0));
    int major, minor;
    CU(cuDeviceGetAttribute(&major, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, dev));
    CU(cuDeviceGetAttribute(&minor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, dev));
    CUcontext ctx;
    CU(cuDevicePrimaryCtxRetain(&ctx, dev));          // 和 Runtime API 用的是同一个上下文
    CU(cuCtxSetCurrent(ctx));
    // ANCHOR_END: driver
    std::string sm = "sm_" + std::to_string(major * 10 + minor);
    std::string compute = "compute_" + std::to_string(major * 10 + minor);

    CUdeviceptr d_c, d_x, d_y;
    CU(cuMemAlloc(&d_c, c.size() * sizeof(float)));
    CU(cuMemAlloc(&d_x, n * sizeof(float)));
    CU(cuMemAlloc(&d_y, n * sizeof(float)));
    CU(cuMemcpyHtoD(d_c, c.data(), c.size() * sizeof(float)));
    CU(cuMemcpyHtoD(d_x, x.data(), n * sizeof(float)));
    CUevent e0, e1;
    CU(cuEventCreate(&e0, CU_EVENT_DEFAULT));
    CU(cuEventCreate(&e1, CU_EVENT_DEFAULT));

    std::string spec = specialized_src(c);
    printf("%d 个点，%d 次多项式；生成的专用源码 %zu 字节\n", n, degree, spec.size());
    printf("版本  格式               NVRTC        装载      kernel\n");
    struct Case { const char* name; std::string src; const std::string* arch; };
    Case cases[] = {{"通用", generic_src, &compute}, {"通用", generic_src, &sm},
                    {"专用", spec, &compute}, {"专用", spec, &sm}};
    for (Case& cs : cases) {
        double t_rtc;
        std::string image = compile(cs.src, cs.arch->c_str(), &t_rtc);
        // ANCHOR: launch
        double t0 = now_ms();
        CUmodule mod;
        CU(cuModuleLoadData(&mod, image.data()));    // PTX 在这里由驱动 JIT 编译成机器码
        CUfunction f;
        CU(cuModuleGetFunction(&f, mod, "poly"));
        double t_load = now_ms() - t0;
        int nn = n, dd = degree;
        void* args[] = {&nn, &dd, &d_c, &d_x, &d_y};   // 每个参数的地址，按 kernel 的参数顺序
        unsigned grid = (n + 255) / 256;
        CU(cuLaunchKernel(f, grid, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr));   // 预热
        CU(cuEventRecord(e0, nullptr));
        for (int rep = 0; rep < 10; ++rep)
            CU(cuLaunchKernel(f, grid, 1, 1, 256, 1, 1, 0, nullptr, args, nullptr));
        CU(cuEventRecord(e1, nullptr));
        CU(cuEventSynchronize(e1));
        // ANCHOR_END: launch
        float t_kernel;
        CU(cuEventElapsedTime(&t_kernel, e0, e1));
        CU(cuMemcpyDtoH(cs.src == spec ? y.data() : y_ref.data(), d_y, n * sizeof(float)));
        CU(cuModuleUnload(mod));
        printf("%s  %-12s %7.1f ms  %7.2f ms  %7.3f ms\n", cs.name, cs.arch->c_str(), t_rtc, t_load,
               t_kernel / 10);
    }
    float diff = 0;
    for (int i = 0; i < n; ++i) diff = fmaxf(diff, fabsf(y[i] - y_ref[i]));
    printf("两个版本结果的最大差 %g\n", diff);
    CU(cuDevicePrimaryCtxRelease(dev));
}
