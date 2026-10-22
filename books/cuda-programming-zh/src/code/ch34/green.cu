// green.cu: 一个占满 GPU 的后台 kernel 正在运行时，一个很小的“请求” kernel 要等多久才能做完？
// 比较普通 stream、高优先级 stream，以及用 green context 给请求单独留出一部分 SM。
// 请求每隔 5 ms 发一个，共 20 个，都落在后台 kernel 运行期间，每个请求用自己的 stream。
//   nvcc -O2 -arch=native green.cu -o green && ./green
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
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

static double now_ms() {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

__global__ void background(long long ns) {            // 每个 block 占着 SM 忙 ns 纳秒
    long long t0, t;
    asm volatile("mov.u64 %0, %%globaltimer;" : "=l"(t0));
    do asm volatile("mov.u64 %0, %%globaltimer;" : "=l"(t)); while (t - t0 < ns);
}
__global__ void request(float* a) { a[blockIdx.x * blockDim.x + threadIdx.x] += 1; }

const int R = 20;                                    // 请求个数
double done_at[R];                                    // 每个请求完成的主机时间
void CUDART_CB mark_done(void* p) { *(double*)p = now_ms(); }

// 后台 kernel（如果有）运行期间，每隔 5 ms 往各自的 stream 里发一个请求。
// 请求延迟 = 从发出请求到 stream 里紧跟着它的主机函数被调用，包括排队等 SM 和执行。
void trial(const char* name, cudaStream_t bg, const std::vector<cudaStream_t>& req, float* a, int bg_blocks) {
    cudaEvent_t b0, b1;
    CHECK(cudaEventCreate(&b0));
    CHECK(cudaEventCreate(&b1));
    if (bg) {
        CHECK(cudaEventRecord(b0, bg));
        background<<<bg_blocks, 768, 0, bg>>>(2 * 1000000LL);   // 每个 block 2 ms
        CHECK(cudaEventRecord(b1, bg));
    }
    std::vector<double> lat(R), sent(R);
    for (int i = 0; i < R; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        sent[i] = now_ms();
        request<<<8, 128, 0, req[i]>>>(a);
        CHECK(cudaLaunchHostFunc(req[i], mark_done, &done_at[i]));
    }
    CHECK(cudaDeviceSynchronize());
    for (int i = 0; i < R; ++i) lat[i] = done_at[i] - sent[i];
    std::sort(lat.begin(), lat.end());
    printf("%s  请求延迟 中位数 %7.3f ms，最大 %7.3f ms", name, lat[R / 2], lat[R - 1]);
    if (bg) {
        float bg_ms;
        CHECK(cudaEventElapsedTime(&bg_ms, b0, b1));
        printf("；后台 kernel %6.1f ms", bg_ms);
    }
    printf("\n");
}

int main() {
    float* a;
    CHECK(cudaMalloc(&a, 8 * 128 * sizeof(float)));
    int sms;
    CHECK(cudaDeviceGetAttribute(&sms, cudaDevAttrMultiProcessorCount, 0));
    const int bg_blocks = sms * 2 * 75;               // 每个 SM 同时容纳 2 个 block，占满整个 GPU 约 150 ms

    // ANCHOR: green
    cudaDevResource all, small, rest;
    CHECK(cudaDeviceGetDevResource(0, &all, cudaDevResourceTypeSm));
    unsigned groups = 1;
    CHECK(cudaDevSmResourceSplitByCount(&small, &groups, &all, &rest, 0, 4));   // 至少 4 个 SM 给请求
    printf("共 %u 个 SM，最小分区 %u 个、按 %u 个对齐；分给请求 %u 个，后台 %u 个\n", all.sm.smCount,
           all.sm.minSmPartitionSize, all.sm.smCoscheduledAlignment, small.sm.smCount, rest.sm.smCount);
    cudaDevResourceDesc_t d_small, d_rest;
    CHECK(cudaDevResourceGenerateDesc(&d_small, &small, 1));
    CHECK(cudaDevResourceGenerateDesc(&d_rest, &rest, 1));
    cudaExecutionContext_t ctx_small, ctx_rest;
    CHECK(cudaGreenCtxCreate(&ctx_small, d_small, 0, 0));
    CHECK(cudaGreenCtxCreate(&ctx_rest, d_rest, 0, 0));
    cudaStream_t g_bg;                                // 这个 stream 上的 kernel 只能用 ctx_rest 的 SM
    CHECK(cudaExecutionCtxStreamCreate(&g_bg, ctx_rest, cudaStreamNonBlocking, 0));
    // ANCHOR_END: green

    cudaStream_t bg;
    std::vector<cudaStream_t> req(R), req_hi(R), req_green(R);
    int lo, hi;
    CHECK(cudaDeviceGetStreamPriorityRange(&lo, &hi));
    CHECK(cudaStreamCreateWithPriority(&bg, cudaStreamNonBlocking, lo));
    for (int i = 0; i < R; ++i) {
        CHECK(cudaStreamCreateWithPriority(&req[i], cudaStreamNonBlocking, lo));
        CHECK(cudaStreamCreateWithPriority(&req_hi[i], cudaStreamNonBlocking, hi));
        CHECK(cudaExecutionCtxStreamCreate(&req_green[i], ctx_small, cudaStreamNonBlocking, 0));
    }
    request<<<8, 128>>>(a);                           // 预热
    background<<<1, 32>>>(1000);
    CHECK(cudaDeviceSynchronize());

    // ANCHOR: trials
    trial("GPU 空闲                ", nullptr, req, a, 0);
    trial("普通 stream             ", bg, req, a, bg_blocks);
    trial("请求用高优先级 stream   ", bg, req_hi, a, bg_blocks);
    trial("green context 各用各的 SM", g_bg, req_green, a, bg_blocks);
    // ANCHOR_END: trials
    CHECK(cudaExecutionCtxDestroy(ctx_small));
    CHECK(cudaExecutionCtxDestroy(ctx_rest));
}
