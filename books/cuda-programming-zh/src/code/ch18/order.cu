// order.cu: GPU 正忙时 cudaFree 会让 host 等多久；流序分配器什么时候复用刚释放的内存。
//   nvcc -O2 -arch=native order.cu -o order && ./order
#include <chrono>
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

constexpr size_t BYTES = size_t(64) << 20;

// 跑十几毫秒的 kernel，用来让 GPU 保持忙碌。
__global__ void busy(float* out) {
    float a = threadIdx.x;
    for (int k = 0; k < 10'000'000; ++k) a = a * 0.9999f + 0.5f;
    out[blockIdx.x * blockDim.x + threadIdx.x] = a;
}

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

int main() {
    float* out;
    CHECK(cudaMalloc(&out, 26 * 256 * sizeof(float)));
    cudaStream_t s1, s2;
    CHECK(cudaStreamCreateWithFlags(&s1, cudaStreamNonBlocking));
    CHECK(cudaStreamCreateWithFlags(&s2, cudaStreamNonBlocking));
    busy<<<26, 256, 0, s1>>>(out);               // 预热
    CHECK(cudaDeviceSynchronize());

    // ANCHOR: blocking
    // GPU 在 s1 上忙着的时候，释放一块和它毫无关系的内存。
    float *a, *b;
    CHECK(cudaMalloc(&a, BYTES));
    CHECK(cudaMallocAsync(&b, BYTES, s2));
    CHECK(cudaStreamSynchronize(s2));
    busy<<<26, 256, 0, s1>>>(out);
    double t0 = now_ms();
    CHECK(cudaFree(a));                          // 会等 GPU 上所有工作都完成
    double t1 = now_ms();
    busy<<<26, 256, 0, s1>>>(out);
    double t2 = now_ms();
    CHECK(cudaFreeAsync(b, s2));                 // 只是排进 s2，马上返回
    double t3 = now_ms();
    // ANCHOR_END: blocking
    CHECK(cudaDeviceSynchronize());
    printf("GPU 忙时，cudaFree 让 host 等了 %6.2f ms，cudaFreeAsync 只用了 %6.3f ms\n\n", t1 - t0, t3 - t2);

    // ANCHOR: reuse
    // 在 s1 上分配、使用、释放，然后分别在 s1、s2 上再分配同样大小的内存，看拿到的是不是同一块。
    cudaEvent_t freed;
    CHECK(cudaEventCreateWithFlags(&freed, cudaEventDisableTiming));
    float *p, *q;
    CHECK(cudaMallocAsync(&p, BYTES, s1));
    busy<<<26, 256, 0, s1>>>(out);               // 这时 p 还在被使用（假装 busy 用到了它）
    CHECK(cudaFreeAsync(p, s1));
    CHECK(cudaEventRecord(freed, s1));

    CHECK(cudaMallocAsync(&q, BYTES, s1));       // 同一条 stream：释放排在前面，可以直接复用
    printf("同一条 stream 上再分配：       %s\n", q == p ? "复用了刚释放的那块" : "拿到另一块");
    CHECK(cudaFreeAsync(q, s1));
    CHECK(cudaEventRecord(freed, s1));

    float* r;
    CHECK(cudaMallocAsync(&r, BYTES, s2));       // 另一条 stream，没有依赖：不能用
    printf("另一条 stream，没有依赖：      %s\n", r == p ? "复用了刚释放的那块" : "拿到另一块");

    CHECK(cudaStreamWaitEvent(s2, freed));       // 建立依赖：s2 之后的工作排在那次释放之后
    float* t;
    CHECK(cudaMallocAsync(&t, BYTES, s2));
    printf("另一条 stream，等待释放的 event：%s\n", t == p ? "复用了刚释放的那块" : "拿到另一块");
    CHECK(cudaFreeAsync(r, s2));
    CHECK(cudaFreeAsync(t, s2));
    // ANCHOR_END: reuse
    CHECK(cudaDeviceSynchronize());
    CHECK(cudaFree(out));
}
