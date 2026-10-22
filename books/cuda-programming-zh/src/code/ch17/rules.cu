// rules.cu: stream 捕获期间，哪些调用可以做，哪些会让捕获失败；跨 stream 的 fork-join 捕获成什么样的 graph。
//   nvcc -O2 -arch=native rules.cu -o rules && ./rules
#include <cstdio>
#include <cstdlib>
#include <functional>
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

__global__ void work(float* x) { x[threadIdx.x] += 1.0f; }

cudaStream_t s, other, blocking;
float* d;

// 开始捕获，执行一个操作，结束捕获，报告操作本身和结束捕获各返回什么。
void try_in_capture(const char* what, std::function<cudaError_t()> op, cudaStream_t cs = s) {
    CHECK(cudaStreamBeginCapture(cs, cudaStreamCaptureModeGlobal));
    work<<<1, 32, 0, cs>>>(d);
    cudaError_t e1 = op();
    cudaGraph_t g = nullptr;
    cudaError_t e2 = cudaStreamEndCapture(cs, &g);
    printf("%-36s %-36s %s\n", cudaGetErrorName(e1), cudaGetErrorName(e2), what);
    if (g) CHECK(cudaGraphDestroy(g));
    cudaGetLastError();                        // 清掉这次留下的错误状态
}

int main() {
    CHECK(cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking));
    CHECK(cudaStreamCreateWithFlags(&other, cudaStreamNonBlocking));
    CHECK(cudaStreamCreate(&blocking));        // 普通 stream，会和默认 stream 互相等待
    CHECK(cudaMalloc(&d, 32 * sizeof(float)));
    float h[32];
    float* p;

    printf("这个调用返回                         结束捕获时返回                       捕获期间做的事\n");
    try_in_capture("cudaMemsetAsync（同一条 stream）", [&] { return cudaMemsetAsync(d, 0, 128, s); });
    try_in_capture("cudaMallocAsync（同一条 stream）", [&] { return cudaMallocAsync(&p, 128, s); });
    try_in_capture("cudaMalloc", [&] { return cudaMalloc(&p, 128); });
    try_in_capture("cudaMemcpy（同步拷贝）", [&] { return cudaMemcpy(h, d, 128, cudaMemcpyDeviceToHost); });
    try_in_capture("cudaStreamSynchronize", [&] { return cudaStreamSynchronize(s); });
    try_in_capture("在默认 stream 上启动 kernel", [&] { work<<<1, 32>>>(d); return cudaGetLastError(); });
    try_in_capture("在另一条 stream 上启动 kernel", [&] { work<<<1, 32, 0, other>>>(d); return cudaGetLastError(); });
    try_in_capture("默认 stream 启动（捕获普通 stream）", [&] { work<<<1, 32>>>(d); return cudaGetLastError(); },
                   blocking);

    // 捕获期间在不相干的 stream 上启动的 kernel，是当场执行的，并没有进 graph。
    // ANCHOR: eager
    CHECK(cudaMemset(d, 0, 32 * sizeof(float)));
    CHECK(cudaDeviceSynchronize());
    cudaGraph_t probe;
    CHECK(cudaStreamBeginCapture(s, cudaStreamCaptureModeGlobal));
    work<<<1, 32, 0, s>>>(d);                  // 被记录进 graph，此刻不执行
    work<<<1, 32, 0, other>>>(d);              // other 不在捕获中：立刻执行
    CHECK(cudaStreamEndCapture(s, &probe));
    CHECK(cudaStreamSynchronize(other));
    CHECK(cudaMemcpy(h, d, sizeof(h), cudaMemcpyDeviceToHost));
    printf("\n捕获结束、graph 还没启动时，d[0] = %.0f\n", h[0]);
    // ANCHOR_END: eager
    CHECK(cudaGraphDestroy(probe));

    // ANCHOR: forkjoin
    // 跨两条 stream 的 fork-join：other 通过 event 加入捕获，最后再汇合回 s。
    cudaEvent_t fork, join;
    CHECK(cudaEventCreateWithFlags(&fork, cudaEventDisableTiming));
    CHECK(cudaEventCreateWithFlags(&join, cudaEventDisableTiming));
    cudaGraph_t g;
    CHECK(cudaStreamBeginCapture(s, cudaStreamCaptureModeGlobal));
    work<<<1, 32, 0, s>>>(d);                  // A
    CHECK(cudaEventRecord(fork, s));
    CHECK(cudaStreamWaitEvent(other, fork));   // other 从这里开始也处于捕获状态
    work<<<1, 32, 0, s>>>(d);                  // B，在 s 上
    work<<<1, 32, 0, other>>>(d);              // C，在 other 上，和 B 并行
    CHECK(cudaEventRecord(join, other));
    CHECK(cudaStreamWaitEvent(s, join));       // 汇合
    work<<<1, 32, 0, s>>>(d);                  // D，等 B 和 C
    CHECK(cudaStreamEndCapture(s, &g));
    // ANCHOR_END: forkjoin
    size_t nodes = 0, edges = 0;
    CHECK(cudaGraphGetNodes(g, nullptr, &nodes));
    CHECK(cudaGraphGetEdges(g, nullptr, nullptr, nullptr, &edges));
    printf("\nfork-join 捕获得到的 graph：%zu 个节点，%zu 条边\n", nodes, edges);
    CHECK(cudaGraphDestroy(g));
    CHECK(cudaFree(d));
}
