// waiting.cu: host 等一个 200 ms 左右的 kernel 时，CPU 在干什么。
//   nvcc -O2 -arch=native waiting.cu -o waiting
//   ./waiting spin | yield | block | event
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <sys/resource.h>

#define CHECK(call)                                                        \
    do {                                                                   \
        cudaError_t err_ = (call);                                         \
        if (err_ != cudaSuccess) {                                         \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__,        \
                    cudaGetErrorName(err_), cudaGetErrorString(err_));     \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

__global__ void busy(float* out, int iters) {
    float a = threadIdx.x;
    for (int k = 0; k < iters; ++k) a = a * 0.9999f + 0.5f;
    out[blockIdx.x * blockDim.x + threadIdx.x] = a;
}

double cpu_seconds() {                       // 这个进程到目前为止用掉的 CPU 时间
    rusage u;
    getrusage(RUSAGE_SELF, &u);
    return u.ru_utime.tv_sec + u.ru_stime.tv_sec + (u.ru_utime.tv_usec + u.ru_stime.tv_usec) / 1e6;
}

int main(int argc, char** argv) {
    const char* mode = argc > 1 ? argv[1] : "spin";
    // ANCHOR: flags
    // 设备标志要在 context 创建之前设置，决定 host 线程等 GPU 时怎么等。
    if (!strcmp(mode, "yield")) CHECK(cudaSetDeviceFlags(cudaDeviceScheduleYield));
    if (!strcmp(mode, "block")) CHECK(cudaSetDeviceFlags(cudaDeviceScheduleBlockingSync));
    // ANCHOR_END: flags
    float* out;
    CHECK(cudaMalloc(&out, 26 * 6 * 256 * sizeof(float)));
    busy<<<26 * 6, 256>>>(out, 1000);        // 预热
    CHECK(cudaDeviceSynchronize());

    // ANCHOR: event
    cudaEvent_t done;
    if (!strcmp(mode, "event"))              // 只让这一个 event 的等待方式变成阻塞
        CHECK(cudaEventCreateWithFlags(&done, cudaEventBlockingSync | cudaEventDisableTiming));
    else
        CHECK(cudaEventCreateWithFlags(&done, cudaEventDisableTiming));
    // ANCHOR_END: event

    double c0 = cpu_seconds();
    busy<<<26 * 6, 256>>>(out, 50'000'000);
    CHECK(cudaEventRecord(done));
    // ANCHOR: wait
    if (!strcmp(mode, "event"))
        CHECK(cudaEventSynchronize(done));   // 等 event：方式由 event 自己的标志决定
    else
        CHECK(cudaDeviceSynchronize());      // 等整个设备：方式由设备标志决定
    // ANCHOR_END: wait
    double cpu = cpu_seconds() - c0;
    // 再看很短的等待：1000 次“启动一个几微秒的 kernel，等它结束”。
    auto t1 = std::chrono::steady_clock::now();
    for (int r = 0; r < 1000; ++r) {
        busy<<<1, 32>>>(out, 100);
        CHECK(cudaEventRecord(done));
        if (!strcmp(mode, "event")) CHECK(cudaEventSynchronize(done));
        else CHECK(cudaDeviceSynchronize());
    }
    double each = std::chrono::duration<double>(std::chrono::steady_clock::now() - t1).count() / 1000;
    printf("%-6s 等 270 ms 的 kernel：CPU 用掉 %3.0f ms；短 kernel 每次启动加等待 %5.1f us\n",
           mode, cpu * 1e3, each * 1e6);
    CHECK(cudaFree(out));
}
