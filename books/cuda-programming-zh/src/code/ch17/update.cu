// update.cu: graph 已经实例化，但每次启动要换一块缓冲区。比较三种改参数的办法各要多久。
//   nvcc -O2 -arch=native update.cu -o update && ./update
#include <chrono>
#include <cstdio>
#include <cstdlib>
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

constexpr int N = 26 * 256;
constexpr int KERNELS = 20;
constexpr int ROUNDS = 200;

__global__ void update(float* x, float a, float b) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < N) x[i] = x[i] * a + b;
}

void step(float* x, cudaStream_t s) {
    for (int k = 0; k < KERNELS; ++k) update<<<N / 256, 256, 0, s>>>(x, 0.999f, 0.001f * k);
}

cudaGraph_t capture(float* x, cudaStream_t s) {
    cudaGraph_t g;
    CHECK(cudaStreamBeginCapture(s, cudaStreamCaptureModeGlobal));
    step(x, s);
    CHECK(cudaStreamEndCapture(s, &g));
    return g;
}

double now_us() {
    using namespace std::chrono;
    return duration<double, std::micro>(steady_clock::now().time_since_epoch()).count();
}

int main() {
    float* buf[2];
    for (auto& b : buf) {
        CHECK(cudaMalloc(&b, N * sizeof(float)));
        CHECK(cudaMemset(b, 0, N * sizeof(float)));
    }
    cudaStream_t s;
    CHECK(cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking));
    cudaGraph_t graph = capture(buf[0], s);
    cudaGraphExec_t exec;
    CHECK(cudaGraphInstantiate(&exec, graph, 0));
    CHECK(cudaGraphLaunch(exec, s));
    CHECK(cudaStreamSynchronize(s));

    // 办法一：每次重新捕获、重新实例化。
    double t0 = now_us();
    for (int r = 0; r < ROUNDS; ++r) {
        cudaGraph_t g = capture(buf[r % 2], s);
        cudaGraphExec_t e;
        CHECK(cudaGraphInstantiate(&e, g, 0));
        CHECK(cudaGraphLaunch(e, s));
        CHECK(cudaStreamSynchronize(s));
        CHECK(cudaGraphExecDestroy(e));
        CHECK(cudaGraphDestroy(g));
    }
    double t1 = now_us();

    // ANCHOR: exec_update
    // 办法二：重新捕获，但用 cudaGraphExecUpdate 把新 graph 的参数灌进已有的实例。
    for (int r = 0; r < ROUNDS; ++r) {
        cudaGraph_t g = capture(buf[r % 2], s);
        cudaGraphExecUpdateResultInfo info;
        CHECK(cudaGraphExecUpdate(exec, g, &info));   // 拓扑不变时成功，否则要重新实例化
        CHECK(cudaGraphLaunch(exec, s));
        CHECK(cudaStreamSynchronize(s));
        CHECK(cudaGraphDestroy(g));
    }
    // ANCHOR_END: exec_update
    double t2 = now_us();

    // ANCHOR: node_update
    // 办法三：直接修改实例里每个 kernel 节点的参数，不捕获、不实例化。
    size_t count = 0;
    CHECK(cudaGraphGetNodes(graph, nullptr, &count));
    std::vector<cudaGraphNode_t> nodes(count);
    CHECK(cudaGraphGetNodes(graph, nodes.data(), &count));
    std::vector<float> bs(count);
    for (size_t k = 0; k < count; ++k) {             // 记下每个节点原来的参数 b
        cudaKernelNodeParams p;
        CHECK(cudaGraphKernelNodeGetParams(nodes[k], &p));
        bs[k] = *static_cast<float*>(p.kernelParams[2]);
    }
    for (int r = 0; r < ROUNDS; ++r) {
        float* x = buf[r % 2];
        float a = 0.999f;
        for (size_t k = 0; k < count; ++k) {
            cudaKernelNodeParams p;
            CHECK(cudaGraphKernelNodeGetParams(nodes[k], &p));
            void* args[] = {&x, &a, &bs[k]};         // 只换第一个参数
            p.kernelParams = args;
            CHECK(cudaGraphExecKernelNodeSetParams(exec, nodes[k], &p));
        }
        CHECK(cudaGraphLaunch(exec, s));
        CHECK(cudaStreamSynchronize(s));
    }
    // ANCHOR_END: node_update
    double t3 = now_us();

    // 参照：参数不变，只是启动。
    for (int r = 0; r < ROUNDS; ++r) {
        CHECK(cudaGraphLaunch(exec, s));
        CHECK(cudaStreamSynchronize(s));
    }
    double t4 = now_us();

    printf("每轮（20 个 kernel，换一次参数，启动并等待）：\n");
    printf("  重新捕获 + 重新实例化   %6.1f us\n", (t1 - t0) / ROUNDS);
    printf("  重新捕获 + ExecUpdate   %6.1f us\n", (t2 - t1) / ROUNDS);
    printf("  逐个修改节点参数        %6.1f us\n", (t3 - t2) / ROUNDS);
    printf("  参照：不改参数          %6.1f us\n", (t4 - t3) / ROUNDS);
    CHECK(cudaGraphExecDestroy(exec));
    CHECK(cudaGraphDestroy(graph));
    for (auto b : buf) CHECK(cudaFree(b));
}
