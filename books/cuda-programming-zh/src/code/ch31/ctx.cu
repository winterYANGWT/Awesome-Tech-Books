// ctx.cu: Runtime API 和 Driver API 混用。两者用的是同一个主上下文，指针、stream、kernel 可以互换；
// 但用 cuCtxCreate 自己建的上下文会成为当前上下文，之后的 Runtime API 调用都落到它上面，
// 它被销毁时，这些内存也跟着没了。
//   nvcc -O2 -arch=native ctx.cu -lcuda -o ctx && ./ctx
// NVCCFLAGS: -lcuda
#include <cuda.h>
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
#define CU(call)                                                                   \
    do {                                                                           \
        CUresult r_ = (call);                                                      \
        if (r_ != CUDA_SUCCESS) {                                                  \
            const char *name, *msg;                                                \
            cuGetErrorName(r_, &name);                                             \
            cuGetErrorString(r_, &msg);                                            \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__, name, msg);    \
            exit(1);                                                               \
        }                                                                          \
    } while (0)

__global__ void add_one(float* a, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) a[i] += 1;
}

CUcontext owner(const void* p) {                      // 这块内存属于哪个上下文
    CUcontext c;
    CU(cuPointerGetAttribute(&c, CU_POINTER_ATTRIBUTE_CONTEXT, (CUdeviceptr)p));
    return c;
}

int main() {
    const int n = 1 << 20;
    // ANCHOR: primary
    float* a;
    CHECK(cudaMalloc(&a, n * sizeof(float)));         // Runtime API 在这里初始化设备 0 的主上下文
    CHECK(cudaMemset(a, 0, n * sizeof(float)));
    CUdevice dev;
    CUcontext primary, current;
    CU(cuDeviceGet(&dev, 0));
    CU(cuDevicePrimaryCtxRetain(&primary, dev));
    CU(cuCtxGetCurrent(&current));
    printf("主上下文 %p，当前上下文 %p，a 属于 %p\n", (void*)primary, (void*)current, (void*)owner(a));

    // Runtime 的 kernel 可以取出 Driver API 的句柄，用 cuLaunchKernel 启动；指针和 stream 直接通用。
    CUfunction f;
    CHECK(cudaGetFuncBySymbol(&f, (const void*)add_one));
    cudaStream_t s;
    CHECK(cudaStreamCreate(&s));
    int nn = n;
    void* args[] = {&a, &nn};
    CU(cuLaunchKernel(f, n / 256, 1, 1, 256, 1, 1, 0, (CUstream)s, args, nullptr));
    float v;
    CHECK(cudaMemcpyAsync(&v, a + 12345, sizeof v, cudaMemcpyDeviceToHost, s));
    CHECK(cudaStreamSynchronize(s));
    printf("cuLaunchKernel 启动 Runtime 的 kernel 以后 a[12345] = %g\n", v);
    // ANCHOR_END: primary

    // ANCHOR: own
    CUcontext mine;
    CU(cuCtxCreate(&mine, nullptr, 0, dev));          // 新建的上下文同时成为当前上下文
    float* b;
    CHECK(cudaMalloc(&b, n * sizeof(float)));         // Runtime API 用的是当前上下文，不是主上下文
    printf("\n自建上下文 %p，之后 cudaMalloc 的 b 属于 %p\n", (void*)mine, (void*)owner(b));
    // ANCHOR_END: own

    // ANCHOR: restore
    CU(cuCtxDestroy(mine));                           // b 随着上下文一起被释放了
    CU(cuCtxSetCurrent(primary));                     // 换回主上下文
    cudaError_t e = cudaMemset(b, 0, n * sizeof(float));
    printf("\n销毁自建上下文以后 cudaMemset(b)：%s\n", cudaGetErrorName(e));
    e = cudaMemcpy(&v, a + 12345, sizeof v, cudaMemcpyDeviceToHost);
    printf("接着在主上下文里读 a：%s\n", cudaGetErrorName(e));
    CU(cuDevicePrimaryCtxRelease(dev));
    CHECK(cudaDeviceReset());                         // 销毁主上下文，下一次调用时重新建立
    e = cudaMalloc(&a, n * sizeof(float));
    printf("cudaDeviceReset 以后重新分配：%s\n", cudaGetErrorName(e));
    // ANCHOR_END: restore
}
