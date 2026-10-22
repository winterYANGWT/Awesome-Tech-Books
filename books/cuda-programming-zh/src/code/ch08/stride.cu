// stride.cu: 每个线程读一个 float，只改相邻线程之间的地址间隔（stride）和起始偏移（offset），
// 看有效带宽怎么变。
//   nvcc -O2 -arch=native stride.cu -o stride && ./stride
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

// ANCHOR: kernels
// 第 i 个线程读 in[i * stride]，写 out[i]。stride = 1 时，相邻线程读相邻的 float。
__global__ void read_strided(const float* in, float* out, int stride) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    out[i] = in[size_t(i) * stride];
}

// 第 i 个线程读 in[i + offset]：地址整体错开 offset 个 float。
__global__ void read_offset(const float* in, float* out, int offset) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    out[i] = in[i + offset];
}
// ANCHOR_END: kernels

int main() {
    const int n = 1 << 24;                 // 每次都读写 1600 多万个 float（64 MB），超过 L2
    const int max_stride = 32;
    float *in, *out;
    CHECK(cudaMalloc(&in, size_t(n) * max_stride * sizeof(float)));
    CHECK(cudaMalloc(&out, n * sizeof(float)));
    CHECK(cudaMemset(in, 0, size_t(n) * max_stride * sizeof(float)));
    cudaEvent_t start, stop;
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));
    auto time = [&](auto launch) {
        launch();
        CHECK(cudaGetLastError());
        float best = 1e30f;
        for (int r = 0; r < 5; ++r) {
            CHECK(cudaEventRecord(start));
            launch();
            CHECK(cudaEventRecord(stop));
            CHECK(cudaEventSynchronize(stop));
            float ms;
            CHECK(cudaEventElapsedTime(&ms, start, stop));
            if (ms < best) best = ms;
        }
        return best;
    };
    // 有效带宽只算程序真正需要的字节：读 n 个 float、写 n 个 float。
    auto gbps = [&](float ms) { return 2.0 * n * sizeof(float) / (ms / 1e3) / 1e9; };

    printf("stride   时间      有效带宽\n");
    for (int stride : {1, 2, 4, 8, 16, 32}) {
        float ms = time([&] { read_strided<<<n / 256, 256>>>(in, out, stride); });
        printf("%5d  %6.3f ms  %6.0f GB/s\n", stride, ms, gbps(ms));
    }
    printf("\noffset   时间      有效带宽\n");
    for (int offset : {0, 1, 2, 4, 8, 16, 32}) {
        float ms = time([&] { read_offset<<<n / 256, 256>>>(in, out, offset); });
        printf("%5d  %6.3f ms  %6.0f GB/s\n", offset, ms, gbps(ms));
    }
    CHECK(cudaFree(in));
    CHECK(cudaFree(out));
}
