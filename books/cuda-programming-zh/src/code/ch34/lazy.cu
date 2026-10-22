// lazy.cu: 模块的按需加载（lazy loading）。这个文件里有 200 个 kernel，另一个文件 lazy_late.cu 里还有一个。
// 比较默认的按需加载和 CUDA_MODULE_LOADING=EAGER：建立上下文的时间、显存占用、kernel 第一次启动的时间，
// 以及 GPU 正忙或者某个 stream 在等主机时，第一次用到另一个模块里的 kernel 会怎样。
//   nvcc -O2 -arch=native lazy.cu lazy_late.cu -lcuda -o lazy
//   ./lazy busy && ./lazy host && CUDA_MODULE_LOADING=EAGER ./lazy busy
// NVCCFLAGS: -lcuda
// LINK_WITH: lazy_late.cu
#include <cuda.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
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

static double now_ms() {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

void launch_late(float* a, cudaStream_t s);           // 在 lazy_late.cu 里

// 200 个互不相同的 kernel：取地址就会让模板实例化，每个都编译进可执行文件。
template <int K>
__global__ void work(float* a, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    float x = a[i];
#pragma unroll
    for (int r = 0; r < 16; ++r) x = sinf(x * float(K + 1)) + cosf(x + float(r));
    a[i] = x;
}
using Kernel = void (*)(float*, int);
template <int... K>
std::array<Kernel, sizeof...(K)> make_table(std::integer_sequence<int, K...>) { return {work<K>...}; }
const auto kernels = make_table(std::make_integer_sequence<int, 200>{});

__global__ void busy(long long ns) {                  // 让 GPU 忙一段时间
    long long t0, t;
    asm volatile("mov.u64 %0, %%globaltimer;" : "=l"(t0));
    do asm volatile("mov.u64 %0, %%globaltimer;" : "=l"(t)); while (t - t0 < ns);
}

std::atomic<bool> released{false};
void CUDART_CB wait_for_host(void*) {                 // stream 里的主机函数：等主机线程放行
    while (!released) std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

int main(int argc, char** argv) {
    const char* scenario = argc > 1 ? argv[1] : "busy";
    // ANCHOR: init
    double t0 = now_ms();
    CHECK(cudaFree(0));                               // 建立上下文
    double t_init = now_ms() - t0;
    CUmoduleLoadingMode mode;
    cuModuleGetLoadingMode(&mode);
    size_t free_b, total_b;
    CHECK(cudaMemGetInfo(&free_b, &total_b));
    printf("加载方式 %s：建立上下文 %.1f ms，显存已用 %zu MB\n", mode == CU_MODULE_LAZY_LOADING ? "LAZY" : "EAGER",
           t_init, (total_b - free_b) >> 20);
    // ANCHOR_END: init

    // ANCHOR: first
    float* a;
    CHECK(cudaMalloc(&a, 1 << 20));
    CHECK(cudaMemset(a, 0, 1 << 20));
    double t_launch[2];
    for (double& t : t_launch) {
        t0 = now_ms();
        kernels[7]<<<1024, 256>>>(a, 1 << 18);
        CHECK(cudaGetLastError());
        CHECK(cudaDeviceSynchronize());
        t = now_ms() - t0;
    }
    printf("work<7> 第一次启动到完成 %.3f ms，第二次 %.3f ms\n", t_launch[0], t_launch[1]);
    // ANCHOR_END: first

    // ANCHOR: blocked
    cudaStream_t s1, s2;
    CHECK(cudaStreamCreateWithFlags(&s1, cudaStreamNonBlocking));
    CHECK(cudaStreamCreateWithFlags(&s2, cudaStreamNonBlocking));
    std::thread releaser;
    if (!strcmp(scenario, "busy")) {
        busy<<<1, 1, 0, s1>>>(300 * 1000000LL);       // s1 上的 kernel 要跑 300 ms
    } else {
        CHECK(cudaLaunchHostFunc(s1, wait_for_host, nullptr));   // s1 在等主机
        releaser = std::thread([] {                   // 2 秒后放行，不然就真的死锁了
            std::this_thread::sleep_for(std::chrono::seconds(2));
            released = true;
        });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    t0 = now_ms();
    launch_late(a, s2);                               // 另一个模块里的 kernel，放在另一个 stream 上
    double t_late = now_ms() - t0;
    CHECK(cudaGetLastError());
    printf("%s时，第一次启动另一个模块里的 kernel：启动这一行花了 %.1f ms\n",
           strcmp(scenario, "busy") ? "s1 在等主机放行" : "s1 上的 kernel 还要跑 300 ms", t_late);
    released = true;
    if (releaser.joinable()) releaser.join();
    // ANCHOR_END: blocked
    CHECK(cudaDeviceSynchronize());
}
