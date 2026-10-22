// launch.cu: 每个时间步是 20 个很小的、前后依赖的 kernel，一共 1000 步。
// 比较逐个启动和捕获成 CUDA Graph 以后整体启动。
//   nvcc -O2 -arch=native launch.cu -o launch && ./launch
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#define CHECK(call)                                                        \
    do {                                                                   \
        cudaError_t err_ = (call);                                         \
        if (err_ != cudaSuccess) {                                         \
            fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__,        \
                    cudaGetErrorName(err_), cudaGetErrorString(err_));     \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

constexpr int N = 26 * 256;             // 每个 SM 一个 block 的数据量，kernel 本身只要一两微秒
constexpr int KERNELS = 20;             // 每个时间步的 kernel 数
constexpr int STEPS = 1000;

__global__ void update(float* x, float a, float b) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < N) x[i] = x[i] * a + b;
}

// ANCHOR: step
// 一个时间步：20 个 kernel 依次放进 stream。
void step(float* x, cudaStream_t s) {
    for (int k = 0; k < KERNELS; ++k)
        update<<<N / 256, 256, 0, s>>>(x, 0.999f, 0.001f * k);
}
// ANCHOR_END: step

double now_us() {
    using namespace std::chrono;
    return duration<double, std::micro>(steady_clock::now().time_since_epoch()).count();
}

int main() {
    float* x;
    CHECK(cudaMalloc(&x, N * sizeof(float)));
    CHECK(cudaMemset(x, 0, N * sizeof(float)));
    cudaStream_t s;
    CHECK(cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking));
    step(x, s);                              // 预热
    CHECK(cudaStreamSynchronize(s));

    // 做法一：逐个启动。
    double t0 = now_us();
    for (int i = 0; i < STEPS; ++i) step(x, s);
    double t_submit = now_us() - t0;
    CHECK(cudaStreamSynchronize(s));
    double t_stream = now_us() - t0;

    usleep(100000);                          // 和后面的做法隔开，方便在时间线上区分
    // ANCHOR: capture
    // 做法二：把一个时间步捕获成 graph，实例化一次，以后每步只启动这一个 graph。
    double c0 = now_us();
    cudaGraph_t graph;
    CHECK(cudaStreamBeginCapture(s, cudaStreamCaptureModeGlobal));
    step(x, s);                              // 这些启动只是被记录下来，并不执行
    CHECK(cudaStreamEndCapture(s, &graph));
    double c1 = now_us();
    cudaGraphExec_t exec;
    CHECK(cudaGraphInstantiate(&exec, graph, 0));
    double c2 = now_us();
    // ANCHOR_END: capture
    CHECK(cudaGraphLaunch(exec, s));         // 预热
    CHECK(cudaStreamSynchronize(s));

    // ANCHOR: launch
    double g0 = now_us();
    for (int i = 0; i < STEPS; ++i) CHECK(cudaGraphLaunch(exec, s));
    double g_submit = now_us() - g0;
    CHECK(cudaStreamSynchronize(s));
    double t_graph = now_us() - g0;
    // ANCHOR_END: launch

    usleep(100000);
    // 做法三：一次捕获 10 个时间步（200 个 kernel），启动 100 次。
    cudaGraph_t graph10;
    CHECK(cudaStreamBeginCapture(s, cudaStreamCaptureModeGlobal));
    for (int i = 0; i < 10; ++i) step(x, s);
    CHECK(cudaStreamEndCapture(s, &graph10));
    cudaGraphExec_t exec10;
    CHECK(cudaGraphInstantiate(&exec10, graph10, 0));
    CHECK(cudaGraphLaunch(exec10, s));
    CHECK(cudaStreamSynchronize(s));
    double h0 = now_us();
    for (int i = 0; i < STEPS / 10; ++i) CHECK(cudaGraphLaunch(exec10, s));
    double h_submit = now_us() - h0;
    CHECK(cudaStreamSynchronize(s));
    double t_graph10 = now_us() - h0;

    const double total = double(STEPS) * KERNELS;
    printf("捕获 %.0f us，实例化 %.0f us\n\n", c1 - c0, c2 - c1);
    printf("                        总时间     平均每个 kernel   其中 host 提交\n");
    printf("逐个启动              %7.1f ms   %6.2f us        %7.1f ms\n", t_stream / 1e3, t_stream / total, t_submit / 1e3);
    printf("graph（1 步）         %7.1f ms   %6.2f us        %7.1f ms\n", t_graph / 1e3, t_graph / total, g_submit / 1e3);
    printf("graph（10 步）        %7.1f ms   %6.2f us        %7.1f ms\n", t_graph10 / 1e3, t_graph10 / total, h_submit / 1e3);

    CHECK(cudaGraphExecDestroy(exec));
    CHECK(cudaGraphExecDestroy(exec10));
    CHECK(cudaGraphDestroy(graph));
    CHECK(cudaGraphDestroy(graph10));
    CHECK(cudaFree(x));
}
