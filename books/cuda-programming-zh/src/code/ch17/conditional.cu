// conditional.cu: 一个要迭代到收敛为止的计算。比较每轮都回到主机检查是否收敛，
// 和用 CUDA Graph 的 while 条件节点让 GPU 自己决定什么时候停。
//   nvcc -O2 -arch=native conditional.cu -o conditional && ./conditional
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

constexpr int N = 256;
constexpr float TOL = 1e-3f;

// ANCHOR: step
// 一步迭代：每个元素向目标值靠近千分之一，然后求出所有元素离目标的最大距离。
__device__ bool one_step(float* x) {
    __shared__ float worst[N];
    int t = threadIdx.x;
    x[t] = x[t] * 0.999f + 0.001f * 1.0f;          // 目标值是 1
    worst[t] = fabsf(x[t] - 1.0f);
    __syncthreads();
    for (int s = N / 2; s > 0; s /= 2) {
        if (t < s) worst[t] = fmaxf(worst[t], worst[t + s]);
        __syncthreads();
    }
    return worst[0] < TOL;                          // 是否已经收敛
}

// 做法一用的 kernel：把是否收敛写到显存里，由主机拷回去看。
__global__ void iterate(float* x, int* done, int* steps) {
    bool ok = one_step(x);
    if (threadIdx.x == 0) *done = ok, ++*steps;
}

// 做法二用的 kernel：直接告诉 graph 要不要再来一轮。
__global__ void iterate_in_graph(float* x, cudaGraphConditionalHandle handle, int* steps) {
    bool ok = one_step(x);
    if (threadIdx.x == 0) {
        ++*steps;
        cudaGraphSetConditional(handle, ok ? 0 : 1);   // 0：退出 while 循环
    }
}
// ANCHOR_END: step

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

int main() {
    float* x;
    int *done, *steps, h_done, h_steps;
    CHECK(cudaMalloc(&x, N * sizeof(float)));
    CHECK(cudaMalloc(&done, sizeof(int)));
    CHECK(cudaMalloc(&steps, sizeof(int)));
    cudaStream_t s;
    CHECK(cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking));

    // ANCHOR: host_loop
    // 做法一：每一步之后把收敛标志拷回主机检查。
    CHECK(cudaMemsetAsync(x, 0, N * sizeof(float), s));
    CHECK(cudaMemsetAsync(steps, 0, sizeof(int), s));
    double t0 = now_ms();
    do {
        iterate<<<1, N, 0, s>>>(x, done, steps);
        CHECK(cudaMemcpyAsync(&h_done, done, sizeof(int), cudaMemcpyDeviceToHost, s));
        CHECK(cudaStreamSynchronize(s));
    } while (!h_done);
    double t_host = now_ms() - t0;
    // ANCHOR_END: host_loop
    CHECK(cudaMemcpy(&h_steps, steps, sizeof(int), cudaMemcpyDeviceToHost));
    printf("每步回到主机检查：%5d 步，%6.1f ms，平均每步 %5.2f us\n", h_steps, t_host, t_host * 1e3 / h_steps);

    // ANCHOR: build
    // 做法二：graph 里放一个 while 条件节点，循环体是一个 kernel。
    cudaGraph_t graph;
    CHECK(cudaGraphCreate(&graph, 0));
    cudaGraphConditionalHandle handle;              // kernel 通过它设置条件值
    CHECK(cudaGraphConditionalHandleCreate(&handle, graph, 1, cudaGraphCondAssignDefault));
    cudaGraphNodeParams params = {};
    params.type = cudaGraphNodeTypeConditional;
    params.conditional.handle = handle;
    params.conditional.type = cudaGraphCondTypeWhile;   // 条件值非零就再执行一遍循环体
    params.conditional.size = 1;
    cudaGraphNode_t node;
    CHECK(cudaGraphAddNode(&node, graph, nullptr, nullptr, 0, &params));
    cudaGraph_t body = params.conditional.phGraph_out[0];   // 循环体是一个子 graph
    // 把 kernel 启动直接捕获进循环体。
    CHECK(cudaStreamBeginCaptureToGraph(s, body, nullptr, nullptr, 0, cudaStreamCaptureModeGlobal));
    iterate_in_graph<<<1, N, 0, s>>>(x, handle, steps);
    CHECK(cudaStreamEndCapture(s, &body));
    cudaGraphExec_t exec;
    CHECK(cudaGraphInstantiate(&exec, graph, 0));
    // ANCHOR_END: build

    CHECK(cudaMemsetAsync(x, 0, N * sizeof(float), s));
    CHECK(cudaMemsetAsync(steps, 0, sizeof(int), s));
    CHECK(cudaStreamSynchronize(s));
    double t1 = now_ms();
    CHECK(cudaGraphLaunch(exec, s));                // 一次启动，循环全在 GPU 上完成
    CHECK(cudaStreamSynchronize(s));
    double t_graph = now_ms() - t1;
    CHECK(cudaMemcpy(&h_steps, steps, sizeof(int), cudaMemcpyDeviceToHost));
    printf("graph while 循环：%5d 步，%6.1f ms，平均每步 %5.2f us\n", h_steps, t_graph, t_graph * 1e3 / h_steps);

    CHECK(cudaGraphExecDestroy(exec));
    CHECK(cudaGraphDestroy(graph));
    CHECK(cudaFree(x));
    CHECK(cudaFree(done));
    CHECK(cudaFree(steps));
}
