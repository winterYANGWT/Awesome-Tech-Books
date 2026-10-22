// transfer.cu: 不同大小的拷贝，pageable 和 pinned 内存各能跑多快；分配 pinned 内存要多久。
//   nvcc -O2 -arch=native transfer.cu -o transfer && ./transfer
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

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

int main() {
    const size_t max_bytes = size_t(256) << 20;
    char* pageable = static_cast<char*>(malloc(max_bytes));
    char* pinned;
    CHECK(cudaMallocHost(&pinned, max_bytes));
    char* dev;
    CHECK(cudaMalloc(&dev, max_bytes));
    memset(pageable, 1, max_bytes);
    memset(pinned, 1, max_bytes);

    // 同步拷贝，用主机时钟量从调用到返回的时间，小的拷贝多做几遍取平均。
    auto rate = [&](char* host, bool to_device, size_t bytes) {
        int reps = bytes < (1 << 20) ? 200 : bytes < (64 << 20) ? 20 : 5;
        auto copy = [&] {
            if (to_device) CHECK(cudaMemcpy(dev, host, bytes, cudaMemcpyHostToDevice));
            else CHECK(cudaMemcpy(host, dev, bytes, cudaMemcpyDeviceToHost));
        };
        copy();                                    // 预热
        double t0 = now_ms();
        for (int r = 0; r < reps; ++r) copy();
        double us = (now_ms() - t0) * 1e3 / reps;
        return std::make_pair(us, bytes / us / 1e3);   // 微秒、GB/s
    };

    printf("   大小    pageable H2D       pinned H2D        pageable D2H       pinned D2H\n");
    for (size_t bytes = 4096; bytes <= max_bytes; bytes *= 4) {
        auto a = rate(pageable, true, bytes), b = rate(pinned, true, bytes);
        auto c = rate(pageable, false, bytes), d = rate(pinned, false, bytes);
        if (bytes < (1 << 20)) printf("%5zu KB", bytes >> 10);
        else printf("%5zu MB", bytes >> 20);
        for (auto r : {a, b, c, d}) printf("  %8.1f us %5.1f", r.first, r.second);
        printf("\n");
    }

    printf("\n分配并锁定        cudaMallocHost   cudaHostRegister\n");
    for (size_t mb : {1, 16, 256, 1024}) {
        size_t bytes = mb << 20;
        double t0 = now_ms();
        char* p;
        CHECK(cudaMallocHost(&p, bytes));
        double t1 = now_ms();
        CHECK(cudaFreeHost(p));
        char* q = static_cast<char*>(malloc(bytes));
        memset(q, 0, bytes);                       // 先把页真正分配出来
        double t2 = now_ms();
        CHECK(cudaHostRegister(q, bytes, cudaHostRegisterDefault));
        double t3 = now_ms();
        CHECK(cudaHostUnregister(q));
        free(q);
        printf("%6zu MB  %12.1f ms  %14.1f ms\n", mb, t1 - t0, t3 - t2);
    }
    free(pageable);
    CHECK(cudaFreeHost(pinned));
    CHECK(cudaFree(dev));
}
