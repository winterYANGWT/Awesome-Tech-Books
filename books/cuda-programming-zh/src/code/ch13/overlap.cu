// overlap.cu: 同一份工作（拷进去、算、拷回来）的几种提交方式，各跑一遍计时。
//   nvcc -O2 -arch=native overlap.cu -o overlap && ./overlap
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

constexpr size_t N = size_t(64) << 20;   // 64M 个 float，256 MiB
constexpr int ITERS = 140;               // 每个元素的计算量，调到 kernel 和一次拷贝差不多长

// ANCHOR: kernel
__global__ void transform(const float* in, float* out, size_t n) {
    size_t i = blockIdx.x * size_t(blockDim.x) + threadIdx.x;
    if (i >= n) return;
    float x = in[i];
    for (int k = 0; k < ITERS; ++k) x = x * 0.999f + 0.001f * sqrtf(x + 1.0f);
    out[i] = x;
}

void launch(const float* in, float* out, size_t n, cudaStream_t s) {
    unsigned blocks = unsigned((n + 255) / 256);
    transform<<<blocks, 256, 0, s>>>(in, out, n);
}
// ANCHOR_END: kernel

double g_submit_ms = 0;   // run_streams 里 host 花在提交工作上的时间

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

// ANCHOR: serial
// 版本 1：一次拷进去，算，一次拷回来，全用同步接口。
double run_serial(float* h_in, float* h_out, float* d_in, float* d_out) {
    double t0 = now_ms();
    CHECK(cudaMemcpy(d_in, h_in, N * sizeof(float), cudaMemcpyHostToDevice));
    launch(d_in, d_out, N, 0);
    CHECK(cudaMemcpy(h_out, d_out, N * sizeof(float), cudaMemcpyDeviceToHost));
    return now_ms() - t0;
}
// ANCHOR_END: serial

// ANCHOR: streams
// 版本 2：切成 chunks 块，轮流放进几条 stream，每块各自拷进、计算、拷回。
double run_streams(float* h_in, float* h_out, float* d_in, float* d_out,
                   int chunks, std::vector<cudaStream_t>& streams) {
    size_t per = N / chunks;
    double t0 = now_ms();
    for (int c = 0; c < chunks; ++c) {
        cudaStream_t s = streams[c % streams.size()];
        size_t off = c * per;
        CHECK(cudaMemcpyAsync(d_in + off, h_in + off, per * sizeof(float),
                              cudaMemcpyHostToDevice, s));
        launch(d_in + off, d_out + off, per, s);
        CHECK(cudaMemcpyAsync(h_out + off, d_out + off, per * sizeof(float),
                              cudaMemcpyDeviceToHost, s));
    }
    g_submit_ms = now_ms() - t0;             // 到这里 host 才把工作全部交出去
    CHECK(cudaDeviceSynchronize());
    return now_ms() - t0;
}
// ANCHOR_END: streams

// 和版本 2 一样，只是每块之后在默认 stream 上清零一个计数器。
double run_streams_with_default(float* h_in, float* h_out, float* d_in, float* d_out,
                                int chunks, std::vector<cudaStream_t>& streams,
                                int* d_counter) {
    size_t per = N / chunks;
    double t0 = now_ms();
    for (int c = 0; c < chunks; ++c) {
        cudaStream_t s = streams[c % streams.size()];
        size_t off = c * per;
        CHECK(cudaMemcpyAsync(d_in + off, h_in + off, per * sizeof(float),
                              cudaMemcpyHostToDevice, s));
        launch(d_in + off, d_out + off, per, s);
        CHECK(cudaMemcpyAsync(h_out + off, d_out + off, per * sizeof(float),
                              cudaMemcpyDeviceToHost, s));
        CHECK(cudaMemsetAsync(d_counter, 0, sizeof(int)));   // 默认 stream
    }
    CHECK(cudaDeviceSynchronize());
    return now_ms() - t0;
}

// 分别测三段各自要多久，作为参照。
void measure_parts(float* h_in, float* h_out, float* d_in, float* d_out, const char* label) {
    cudaEvent_t e[4];
    for (auto& ev : e) CHECK(cudaEventCreate(&ev));
    CHECK(cudaEventRecord(e[0]));
    CHECK(cudaMemcpy(d_in, h_in, N * sizeof(float), cudaMemcpyHostToDevice));
    CHECK(cudaEventRecord(e[1]));
    launch(d_in, d_out, N, 0);
    CHECK(cudaEventRecord(e[2]));
    CHECK(cudaMemcpy(h_out, d_out, N * sizeof(float), cudaMemcpyDeviceToHost));
    CHECK(cudaEventRecord(e[3]));
    CHECK(cudaEventSynchronize(e[3]));
    float h2d, kern, d2h;
    CHECK(cudaEventElapsedTime(&h2d, e[0], e[1]));
    CHECK(cudaEventElapsedTime(&kern, e[1], e[2]));
    CHECK(cudaEventElapsedTime(&d2h, e[2], e[3]));
    double gb = N * sizeof(float) / 1e9;
    printf("%-10s H2D %6.1f ms (%4.1f GB/s)   kernel %6.1f ms   D2H %6.1f ms (%4.1f GB/s)\n",
           label, h2d, gb / (h2d / 1e3), kern, d2h, gb / (d2h / 1e3));
    for (auto& ev : e) CHECK(cudaEventDestroy(ev));
}

bool check(const float* in, const float* out) {
    for (size_t i = 0; i < N; i += 4099) {
        float x = in[i];
        for (int k = 0; k < ITERS; ++k) x = x * 0.999f + 0.001f * sqrtf(x + 1.0f);
        if (fabsf(x - out[i]) > 1e-4f * fabsf(x) + 1e-6f) return false;
    }
    return true;
}

int main() {
    cudaDeviceProp prop;
    CHECK(cudaGetDeviceProperties(&prop, 0));
    printf("%s，copy engine %d 个\n", prop.name, prop.asyncEngineCount);

    size_t bytes = N * sizeof(float);
    float *d_in, *d_out;
    int* d_counter;
    CHECK(cudaMalloc(&d_in, bytes));
    CHECK(cudaMalloc(&d_out, bytes));
    CHECK(cudaMalloc(&d_counter, sizeof(int)));

    float* p_in = static_cast<float*>(malloc(bytes));    // pageable
    float* p_out = static_cast<float*>(malloc(bytes));
    float *k_in, *k_out;                                 // pinned
    double t0 = now_ms();
    CHECK(cudaMallocHost(&k_in, bytes));
    printf("cudaMallocHost 分配 256 MiB 用了 %.1f ms\n\n", now_ms() - t0);
    CHECK(cudaMallocHost(&k_out, bytes));
    for (size_t i = 0; i < N; ++i) p_in[i] = float(i % 1000) / 1000.0f;
    memcpy(k_in, p_in, bytes);
    memset(p_out, 0, bytes);   // 先碰一遍，免得第一次写触发缺页算进计时
    memset(k_out, 0, bytes);

    std::vector<cudaStream_t> streams(4);
    for (auto& s : streams) CHECK(cudaStreamCreate(&s));
    std::vector<cudaStream_t> nb_streams(4);
    for (auto& s : nb_streams) CHECK(cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking));

    for (int r = 0; r < 3; ++r)             // 预热：context 初始化、模块加载、GPU 升频
        run_serial(k_in, k_out, d_in, d_out);
    measure_parts(p_in, p_out, d_in, d_out, "pageable");
    measure_parts(k_in, k_out, d_in, d_out, "pinned");
    printf("\n");

    double submit = 0;                      // 最快那一次里 host 花在提交上的时间
    auto best = [&](auto f) {               // 每种跑 5 次取最快的一次
        double t = 1e30;
        for (int r = 0; r < 5; ++r) {
            double x = f();
            if (x < t) t = x, submit = g_submit_ms;
        }
        return t;
    };
    auto report = [](const char* label, double ms) { printf("%7.1f ms  %s\n", ms, label); };

    report("1. 同步，pageable",
           best([&] { return run_serial(p_in, p_out, d_in, d_out); }));
    report("2. 同步，pinned",
           best([&] { return run_serial(k_in, k_out, d_in, d_out); }));
    report("3. 4 条 stream、16 块，pageable",
           best([&] { return run_streams(p_in, p_out, d_in, d_out, 16, streams); }));
    printf("           其中 host 花在提交上 %.1f ms\n", submit);
    report("4. 4 条 stream、16 块，pinned",
           best([&] { return run_streams(k_in, k_out, d_in, d_out, 16, streams); }));
    printf("           其中 host 花在提交上 %.1f ms\n", submit);
    if (!check(k_in, k_out)) { printf("结果不对\n"); return 1; }
    report("5. 同 4，每块后加一次默认 stream 上的 memset",
           best([&] { return run_streams_with_default(k_in, k_out, d_in, d_out, 16,
                                                      streams, d_counter); }));
    report("6. 同 5，stream 改用 cudaStreamNonBlocking 创建",
           best([&] { return run_streams_with_default(k_in, k_out, d_in, d_out, 16,
                                                      nb_streams, d_counter); }));

    printf("\n  块数  4 条 stream，pinned\n");
    for (int chunks : {1, 2, 4, 8, 16, 32, 64, 256, 1024}) {
        double t = best([&] { return run_streams(k_in, k_out, d_in, d_out, chunks, streams); });
        printf("%6d  %7.1f ms\n", chunks, t);
    }

    printf("\nstream 数  16 块，pinned\n");
    for (int count : {1, 2, 3, 4, 8}) {
        std::vector<cudaStream_t> ss(count);
        for (auto& s : ss) CHECK(cudaStreamCreate(&s));
        double t = best([&] { return run_streams(k_in, k_out, d_in, d_out, 16, ss); });
        printf("%6d    %7.1f ms\n", count, t);
        for (auto& s : ss) CHECK(cudaStreamDestroy(s));
    }

    for (auto& s : streams) CHECK(cudaStreamDestroy(s));
    for (auto& s : nb_streams) CHECK(cudaStreamDestroy(s));
    CHECK(cudaFreeHost(k_in));
    CHECK(cudaFreeHost(k_out));
    free(p_in);
    free(p_out);
    CHECK(cudaFree(d_in));
    CHECK(cudaFree(d_out));
    CHECK(cudaFree(d_counter));
}
