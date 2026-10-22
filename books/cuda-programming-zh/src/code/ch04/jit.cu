// jit.cu: 64 个 kernel（同一个模板的 64 个实例），量程序启动和第一次启动各 kernel 的时间。
// 用不同的 -gencode 编译，可以看到 JIT 编译和 JIT 缓存的效果：
//   nvcc -O2 -arch=native jit.cu -o jit_sass
//   nvcc -O2 -gencode arch=compute_80,code=compute_80 jit.cu -o jit_ptx
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <utility>

#define CHECK(call)                                                        \
    do {                                                                   \
        cudaError_t err_ = (call);                                         \
        if (err_ != cudaSuccess) {                                         \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__,        \
                    cudaGetErrorName(err_), cudaGetErrorString(err_));     \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

// 每个实例的循环展开成几百条指令，让编译这些 kernel 需要一点时间。
template <int K>
__global__ void work(float* p, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    float v = p[i];
#pragma unroll
    for (int k = 0; k < 256; ++k) v = v * 0.9999f + float(K + k) * 1e-7f;
    p[i] = v;
}

template <int... K>
void launch_all(float* p, int n, std::integer_sequence<int, K...>) {
    (work<K><<<n / 256, 256>>>(p, n), ...);   // 依次启动 work<0> … work<63>
}

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

int main() {
    const int n = 1 << 16;
    double t0 = now_ms();
    CHECK(cudaFree(0));                       // 初始化 context
    double t1 = now_ms();
    float* p;
    CHECK(cudaMalloc(&p, n * sizeof(float)));
    CHECK(cudaMemset(p, 0, n * sizeof(float)));
    CHECK(cudaDeviceSynchronize());
    double t2 = now_ms();
    launch_all(p, n, std::make_integer_sequence<int, 64>{});
    CHECK(cudaGetLastError());
    CHECK(cudaDeviceSynchronize());
    double t3 = now_ms();
    launch_all(p, n, std::make_integer_sequence<int, 64>{});
    CHECK(cudaDeviceSynchronize());
    double t4 = now_ms();
    printf("初始化 %7.1f ms   第一轮 64 个 kernel %7.1f ms   第二轮 %5.1f ms\n",
           t1 - t0, t3 - t2, t4 - t3);
    CHECK(cudaFree(p));
}
