// solver.cu: 用 cuSOLVER 求对称矩阵的全部特征值和特征向量；再对一个非正定矩阵做 Cholesky 分解，
// 看出错的信息在哪里。
//   nvcc -O2 -arch=native solver.cu -o solver -lcusolver -lcublas && ./solver
// NVCCFLAGS: -lcusolver -lcublas
#include <cublas_v2.h>
#include <cusolverDn.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
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

#define CHECK_SOLVER(call)                                                 \
    do {                                                                   \
        cusolverStatus_t st_ = (call);                                     \
        if (st_ != CUSOLVER_STATUS_SUCCESS) {                              \
            fprintf(stderr, "%s:%d: cuSOLVER 错误 %d\n", __FILE__, __LINE__, \
                    int(st_));                                             \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

int main() {
    const int n = 2048;
    // 随机对称矩阵（对称矩阵按行主序还是列主序存放都一样）。
    std::vector<float> h(size_t(n) * n);
    std::mt19937 rng(9);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j <= i; ++j) h[size_t(i) * n + j] = h[size_t(j) * n + i] = dist(rng);
    float *A, *W;
    int* info;
    CHECK(cudaMalloc(&A, h.size() * sizeof(float)));
    CHECK(cudaMalloc(&W, n * sizeof(float)));
    CHECK(cudaMalloc(&info, sizeof(int)));
    CHECK(cudaMemcpy(A, h.data(), h.size() * sizeof(float), cudaMemcpyHostToDevice));
    cusolverDnHandle_t sh;
    CHECK_SOLVER(cusolverDnCreate(&sh));

    // ANCHOR: syevd
    // 第一步：问需要多大的工作区。第二步：分配好再计算。
    int lwork = 0;
    CHECK_SOLVER(cusolverDnSsyevd_bufferSize(sh, CUSOLVER_EIG_MODE_VECTOR, CUBLAS_FILL_MODE_LOWER, n, A, n, W,
                                             &lwork));
    float* work;
    CHECK(cudaMalloc(&work, lwork * sizeof(float)));
    CHECK_SOLVER(cusolverDnSsyevd(sh, CUSOLVER_EIG_MODE_VECTOR, CUBLAS_FILL_MODE_LOWER, n, A, n, W, work, lwork,
                                  info));                                   // 预热：第一次调用有初始化开销
    CHECK(cudaMemcpy(A, h.data(), h.size() * sizeof(float), cudaMemcpyHostToDevice));   // A 被覆盖了，重新拷
    double t0 = now_ms();
    CHECK_SOLVER(cusolverDnSsyevd(sh, CUSOLVER_EIG_MODE_VECTOR, CUBLAS_FILL_MODE_LOWER, n, A, n, W, work, lwork,
                                  info));
    int h_info;
    CHECK(cudaMemcpy(&h_info, info, sizeof(int), cudaMemcpyDeviceToHost));   // 这一步会等计算完成
    double t_eig = now_ms() - t0;
    // ANCHOR_END: syevd
    printf("%d×%d 对称矩阵的特征分解：%.1f ms，工作区 %.1f MB，devInfo = %d\n", n, n, t_eig,
           lwork * 4.0 / 1048576, h_info);

    // 验证：A·v = λ·v。计算后 A 被特征向量覆盖（第 k 列是第 k 个特征向量），λ 从小到大排列。
    std::vector<float> V(h.size()), lambda(n);
    CHECK(cudaMemcpy(V.data(), A, V.size() * sizeof(float), cudaMemcpyDeviceToHost));
    CHECK(cudaMemcpy(lambda.data(), W, n * sizeof(float), cudaMemcpyDeviceToHost));
    double worst = 0;
    for (int k : {0, n / 2, n - 1}) {
        const float* v = &V[size_t(k) * n];            // 列主序：第 k 列连续存放
        double num = 0, den = 0;
        for (int i = 0; i < n; ++i) {
            double av = 0;
            for (int j = 0; j < n; ++j) av += double(h[size_t(i) * n + j]) * v[j];
            num += (av - lambda[k] * v[i]) * (av - lambda[k] * v[i]);
            den += double(v[i]) * v[i];
        }
        worst = std::fmax(worst, std::sqrt(num / den));
    }
    printf("最小特征值 %.4f，最大特征值 %.4f，‖A·v − λ·v‖ 最大为 %.1e\n\n", lambda[0], lambda[n - 1], worst);

    // ANCHOR: potrf
    // Cholesky 分解要求矩阵正定。这个随机对称矩阵有负特征值，分解必然失败。
    CHECK(cudaMemcpy(A, h.data(), h.size() * sizeof(float), cudaMemcpyHostToDevice));
    CHECK_SOLVER(cusolverDnSpotrf_bufferSize(sh, CUBLAS_FILL_MODE_LOWER, n, A, n, &lwork));
    CHECK(cudaFree(work));
    CHECK(cudaMalloc(&work, lwork * sizeof(float)));
    cusolverStatus_t st = cusolverDnSpotrf(sh, CUBLAS_FILL_MODE_LOWER, n, A, n, work, lwork, info);
    CHECK(cudaMemcpy(&h_info, info, sizeof(int), cudaMemcpyDeviceToHost));
    printf("cusolverDnSpotrf 返回 %s，devInfo = %d\n", st == CUSOLVER_STATUS_SUCCESS ? "成功" : "失败", h_info);
    // ANCHOR_END: potrf

    CHECK_SOLVER(cusolverDnDestroy(sh));
    CHECK(cudaFree(A));
    CHECK(cudaFree(W));
    CHECK(cudaFree(info));
    CHECK(cudaFree(work));
}
