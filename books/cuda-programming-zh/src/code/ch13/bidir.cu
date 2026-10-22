// bidir.cu: 单独测 H2D、D2H 各自的带宽，以及两个方向同时拷贝时的带宽。
//   nvcc -O2 -arch=native bidir.cu -o bidir && ./bidir
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

int main() {
    const size_t bytes = size_t(256) << 20;
    float *h_a, *h_b, *d_a, *d_b;
    CHECK(cudaMallocHost(&h_a, bytes));
    CHECK(cudaMallocHost(&h_b, bytes));
    CHECK(cudaMalloc(&d_a, bytes));
    CHECK(cudaMalloc(&d_b, bytes));
    cudaStream_t up, down;
    CHECK(cudaStreamCreate(&up));
    CHECK(cudaStreamCreate(&down));
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));

    // which: 1 = 只拷上去，2 = 只拷回来，3 = 两个方向同时
    auto run = [&](int which) {
        float best = 1e30f;
        for (int r = 0; r < 5; ++r) {
            CHECK(cudaEventRecord(start));
            if (which & 1) CHECK(cudaMemcpyAsync(d_a, h_a, bytes, cudaMemcpyHostToDevice, up));
            if (which & 2) CHECK(cudaMemcpyAsync(h_b, d_b, bytes, cudaMemcpyDeviceToHost, down));
            CHECK(cudaStreamSynchronize(up));
            CHECK(cudaStreamSynchronize(down));
            CHECK(cudaEventRecord(stop));
            CHECK(cudaEventSynchronize(stop));
            float ms;
            CHECK(cudaEventElapsedTime(&ms, start, stop));
            if (ms < best) best = ms;
        }
        return best;
    };
    double gb = bytes / 1e9;
    float t1 = run(1), t2 = run(2), t3 = run(3);
    printf("只拷上去   %5.1f ms  %4.1f GB/s\n", t1, gb / (t1 / 1e3));
    printf("只拷回来   %5.1f ms  %4.1f GB/s\n", t2, gb / (t2 / 1e3));
    printf("同时双向   %5.1f ms  每个方向 %4.1f GB/s\n", t3, gb / (t3 / 1e3));

    CHECK(cudaFreeHost(h_a));
    CHECK(cudaFreeHost(h_b));
    CHECK(cudaFree(d_a));
    CHECK(cudaFree(d_b));
}
