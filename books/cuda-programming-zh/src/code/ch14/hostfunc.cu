// hostfunc.cu: 每块数据从 GPU 拷回来以后，CPU 要做一点后处理。
// 比较“GPU 全部做完再统一处理”和“用 cudaLaunchHostFunc 在每块完成时就处理”。
//   nvcc -O2 -arch=native hostfunc.cu -o hostfunc && ./hostfunc
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

constexpr int CHUNKS = 16;
constexpr int PER = 1 << 22;                // 每块 400 多万个 float，16 MB

__global__ void transform(const float* in, float* out, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    float v = in[i];
    for (int k = 0; k < 300; ++k) v = v * 0.999f + 0.001f;
    out[i] = v;
}

// ANCHOR: post
struct Chunk {
    const float* data;   // 这一块结果在主机内存里的位置
    double sum;          // 后处理的结果
};

// CPU 上的后处理：这里只是把这一块的结果加起来，代表真实程序里的统计、写文件之类的工作。
void post_process(Chunk* c) {
    double s = 0;
    for (int i = 0; i < PER; ++i) s += c->data[i];
    c->sum = s;
}

// 交给 cudaLaunchHostFunc 的回调：签名固定为 void(void*)。
void CUDART_CB on_chunk_done(void* arg) { post_process(static_cast<Chunk*>(arg)); }
// ANCHOR_END: post

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

int main() {
    size_t bytes = size_t(CHUNKS) * PER * sizeof(float);
    float *h_in, *h_out, *d_in, *d_out;
    CHECK(cudaMallocHost(&h_in, bytes));
    CHECK(cudaMallocHost(&h_out, bytes));
    CHECK(cudaMalloc(&d_in, bytes));
    CHECK(cudaMalloc(&d_out, bytes));
    for (size_t i = 0; i < size_t(CHUNKS) * PER; ++i) h_in[i] = float(i % 1000) / 1000.0f;
    cudaStream_t s[4];
    for (auto& st : s) CHECK(cudaStreamCreateWithFlags(&st, cudaStreamNonBlocking));
    Chunk chunks[CHUNKS];

    // 把第 c 块的拷进、计算、拷回放进 stream st。
    auto enqueue = [&](int c, cudaStream_t st) {
        size_t off = size_t(c) * PER;
        CHECK(cudaMemcpyAsync(d_in + off, h_in + off, PER * sizeof(float), cudaMemcpyHostToDevice, st));
        transform<<<PER / 256, 256, 0, st>>>(d_in + off, d_out + off, PER);
        CHECK(cudaMemcpyAsync(h_out + off, d_out + off, PER * sizeof(float), cudaMemcpyDeviceToHost, st));
        chunks[c].data = h_out + off;
    };
    for (int c = 0; c < CHUNKS; ++c) enqueue(c, s[c % 4]);   // 预热
    CHECK(cudaDeviceSynchronize());

    // 做法一：GPU 上全部做完，再在主线程里逐块处理。
    double t0 = now_ms();
    for (int c = 0; c < CHUNKS; ++c) enqueue(c, s[c % 4]);
    CHECK(cudaDeviceSynchronize());
    double t_gpu = now_ms() - t0;
    for (int c = 0; c < CHUNKS; ++c) post_process(&chunks[c]);
    double t1 = now_ms() - t0;
    double sum1 = 0;
    for (auto& c : chunks) sum1 += c.sum;

    // 做法二：每块拷回来以后，在同一条 stream 里排一个主机回调，GPU 还在算后面几块时 CPU 就开始处理。
    double t2 = now_ms();
    for (int c = 0; c < CHUNKS; ++c) {
        enqueue(c, s[c % 4]);
        // ANCHOR: launch
        CHECK(cudaLaunchHostFunc(s[c % 4], on_chunk_done, &chunks[c]));
        // ANCHOR_END: launch
    }
    CHECK(cudaDeviceSynchronize());
    double t3 = now_ms() - t2;
    double sum2 = 0;
    for (auto& c : chunks) sum2 += c.sum;

    printf("只算 GPU 部分                %6.1f ms\n", t_gpu);
    printf("做法一：做完再统一处理       %6.1f ms\n", t1);
    printf("做法二：每块完成就在回调里处理 %6.1f ms\n", t3);
    printf("两种做法的结果%s\n", sum1 == sum2 ? "相同" : "不同");
    CHECK(cudaFreeHost(h_in));
    CHECK(cudaFreeHost(h_out));
    CHECK(cudaFree(d_in));
    CHECK(cudaFree(d_out));
}
