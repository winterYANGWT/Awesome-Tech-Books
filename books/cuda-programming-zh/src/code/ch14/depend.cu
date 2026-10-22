// depend.cu: 一条 stream 生产数据，另外两条 stream 消费。不建立依赖会读到旧数据；
// 用 event 建立依赖后结果正确，两个消费者仍然可以同时运行。
//   nvcc -O2 -arch=native depend.cu -o depend && ./depend
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
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

constexpr int N = 1 << 20;
constexpr int ITERS = 3000;

// ANCHOR: kernels
// 生产者：只开少量 block，先算一阵（模拟真实的计算），再把整个数组写成 value。
__global__ void produce(float* data, float value) {
    float v = value;
    for (int k = 0; k < 200000; ++k) v = v * 1.0000001f;
    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < N; i += gridDim.x * blockDim.x)
        data[i] = value + (v - v);        // 写进去的就是 value，循环只是为了耗时
}

// 消费者：读出一个元素，做一段计算，写到自己的输出数组里。
__global__ void consume(const float* data, float* out, float scale) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    float v = data[i];
    for (int k = 0; k < ITERS; ++k) v = v * scale + 0.001f;
    out[i] = v;
}
// ANCHOR_END: kernels

// 在主机上算出期望的结果，用 fmaf 保证和 GPU 的乘加完全一致。
float expected(float value, float scale) {
    for (int k = 0; k < ITERS; ++k) value = fmaf(value, scale, 0.001f);
    return value;
}

int count_wrong(const float* d_out, float expect) {
    std::vector<float> h(N);
    CHECK(cudaMemcpy(h.data(), d_out, N * sizeof(float), cudaMemcpyDeviceToHost));
    int wrong = 0;
    for (float v : h) wrong += v != expect;
    return wrong;
}

int main() {
    float *data, *out1, *out2;
    CHECK(cudaMalloc(&data, N * sizeof(float)));
    CHECK(cudaMalloc(&out1, N * sizeof(float)));
    CHECK(cudaMalloc(&out2, N * sizeof(float)));
    CHECK(cudaMemset(data, 0, N * sizeof(float)));
    CHECK(cudaDeviceSynchronize());
    cudaStream_t s0, s1, s2;
    CHECK(cudaStreamCreateWithFlags(&s0, cudaStreamNonBlocking));
    CHECK(cudaStreamCreateWithFlags(&s1, cudaStreamNonBlocking));
    CHECK(cudaStreamCreateWithFlags(&s2, cudaStreamNonBlocking));
    cudaEvent_t produced, done1, done2;
    CHECK(cudaEventCreateWithFlags(&produced, cudaEventDisableTiming));
    CHECK(cudaEventCreateWithFlags(&done1, cudaEventDisableTiming));
    CHECK(cudaEventCreateWithFlags(&done2, cudaEventDisableTiming));
    const int blocks = N / 256, few = 26;   // 生产者只用 26 个 block，GPU 上还有很多空位

    // 第一轮：不建立依赖。生产者在 s0 上把数据写成 1，消费者在 s1 上读。
    // ANCHOR: race
    produce<<<few, 256, 0, s0>>>(data, 1.0f);
    consume<<<blocks, 256, 0, s1>>>(data, out1, 0.999f);   // 和 s0 之间没有任何顺序
    // ANCHOR_END: race
    CHECK(cudaGetLastError());
    CHECK(cudaDeviceSynchronize());
    printf("不建立依赖：%d 个结果是用旧数据算的\n", count_wrong(out1, expected(1.0f, 0.999f)));

    usleep(100000);                          // 和第二轮隔开，方便在时间线上区分
    // 第二轮：fork-join。生产者在 s0 上，两个消费者分别在 s1、s2 上等它，最后 s0 再等两个消费者。
    // ANCHOR: forkjoin
    produce<<<few, 256, 0, s0>>>(data, 2.0f);
    CHECK(cudaEventRecord(produced, s0));          // 在 s0 的这个位置做个记号
    CHECK(cudaStreamWaitEvent(s1, produced));      // s1 之后的工作要等记号之前的工作都做完
    CHECK(cudaStreamWaitEvent(s2, produced));
    consume<<<blocks, 256, 0, s1>>>(data, out1, 0.999f);
    consume<<<blocks, 256, 0, s2>>>(data, out2, 0.998f);
    CHECK(cudaEventRecord(done1, s1));
    CHECK(cudaEventRecord(done2, s2));
    CHECK(cudaStreamWaitEvent(s0, done1));         // s0 等两个消费者都结束
    CHECK(cudaStreamWaitEvent(s0, done2));
    produce<<<few, 256, 0, s0>>>(data, 3.0f);      // 下一轮生产，不会覆盖还在被读的数据
    // ANCHOR_END: forkjoin
    CHECK(cudaGetLastError());
    CHECK(cudaStreamSynchronize(s0));
    printf("用 event 建立依赖：%d 和 %d 个结果是用旧数据算的\n",
           count_wrong(out1, expected(2.0f, 0.999f)), count_wrong(out2, expected(2.0f, 0.998f)));

    CHECK(cudaFree(data));
    CHECK(cudaFree(out1));
    CHECK(cudaFree(out2));
}
