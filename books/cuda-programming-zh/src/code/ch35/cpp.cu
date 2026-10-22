// cpp.cu: 设备代码里能用到哪些 C++：lambda、模板和 concept、libcu++、在设备上创建的对象的虚函数。
// 用 -DSHOW_ERROR=1/2 编译会故意触发两种不允许的写法，看编译器怎么说。
// -DSHOW_ERROR=3 调用主机上构造的对象的虚函数，在作者的机器上 kernel 直接卡死，只能杀掉进程，慎用。
//   nvcc -O2 -arch=native -std=c++20 --extended-lambda cpp.cu -o cpp && ./cpp
// NVCCFLAGS: -std=c++20 --extended-lambda
#include <cuda/std/array>
#include <cuda/std/complex>
#include <cuda/std/span>
#include <cuda/std/tuple>
#include <concepts>
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

// ANCHOR: lambda
// 一个通用的“对每个下标做 f”的 kernel，f 可以是任何可以在设备上调用的东西。
template <class F>
__global__ void for_each_index(int n, F f) {
    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < n; i += gridDim.x * blockDim.x) f(i);
}

// 同一个函数在主机和设备上都能调用。
__host__ __device__ float smooth(float x) { return x * x * (3 - 2 * x); }
// ANCHOR_END: lambda

// ANCHOR: concept
// C++20 的 concept 只在编译期起作用，设备代码里照样能用。
template <std::floating_point T>
__host__ __device__ T horner(cuda::std::span<const T> c, T x) {
    T r = 0;
    for (T ci : c) r = r * x + ci;                    // 范围 for 循环
    return r;
}

// 返回多个值：cuda::std::tuple 加结构化绑定。
__device__ cuda::std::tuple<float, float> min_max(cuda::std::span<const float> v) {
    float lo = v[0], hi = v[0];
    for (float x : v) lo = fminf(lo, x), hi = fmaxf(hi, x);
    return {lo, hi};
}

__global__ void use_libcu(const float* data, int n, float* out) {
    constexpr cuda::std::array<float, 4> coef = {1.0f, -2.0f, 0.5f, 3.0f};
    auto [lo, hi] = min_max(cuda::std::span<const float>(data, n));
    cuda::std::complex<float> z(lo, hi);
    z = z * z + cuda::std::complex<float>(1, 0);
    out[0] = horner(cuda::std::span<const float>(coef), 2.0f);
    out[1] = lo, out[2] = hi, out[3] = z.real(), out[4] = z.imag();
}
// ANCHOR_END: concept

// ANCHOR: virtual
// 有虚函数的类：对象必须在设备上构造，虚函数表指针才指向设备代码。
struct Shape {
    __device__ virtual float area() const = 0;
    __device__ virtual ~Shape() {}
};
struct Circle : Shape {
    float r;
    __device__ Circle(float r) : r(r) {}
    __device__ float area() const override { return 3.14159265f * r * r; }
};
struct Square : Shape {
    float a;
    __device__ Square(float a) : a(a) {}
    __device__ float area() const override { return a * a; }
};
__global__ void shapes(float* out) {
    Shape* s[2] = {new Circle(1.0f), new Square(2.0f)};   // 设备端的 new，从设备堆上分配
    for (int i = 0; i < 2; ++i) out[i] = s[i]->area(), delete s[i];
}
// ANCHOR_END: virtual

#if SHOW_ERROR == 1
__global__ void bad(int* p) {                         // 设备代码里不能抛异常
    if (*p < 0) throw 1;
}
#elif SHOW_ERROR == 2
__global__ void bad(int* p) {                         // std::vector 的成员函数都是主机函数
    std::vector<int> v(4);
    *p = v.size();
}
#elif SHOW_ERROR == 3
struct HostMade {                                     // 在主机上构造、整个拷到设备上的对象
    virtual __host__ __device__ int f() const { return 42; }
};
__global__ void bad(const HostMade* o, int* p) { *p = o->f(); }
#endif

int main() {
    const int n = 1 << 20;
    float *x, *y;
    CHECK(cudaMallocManaged(&x, n * sizeof(float)));
    CHECK(cudaMallocManaged(&y, n * sizeof(float)));
    for (int i = 0; i < n; ++i) x[i] = float(i) / n, y[i] = 1;

    // ANCHOR: use_lambda
    float a = 2;
    for_each_index<<<256, 256>>>(n, [=] __device__(int i) { y[i] = a * smooth(x[i]) + y[i]; });
    CHECK(cudaDeviceSynchronize());
    printf("lambda：y[n/2] = %g，主机上算 %g\n", y[n / 2], a * smooth(x[n / 2]) + 1);
    // ANCHOR_END: use_lambda

    float* out;
    CHECK(cudaMallocManaged(&out, 8 * sizeof(float)));
    use_libcu<<<1, 1>>>(x, 1000, out);
    CHECK(cudaDeviceSynchronize());
    printf("concept + span：horner = %g；tuple：min %g，max %g；complex：(%g, %g)\n", out[0], out[1], out[2],
           out[3], out[4]);
    shapes<<<1, 1>>>(out);
    CHECK(cudaDeviceSynchronize());
    printf("设备上构造的对象调用虚函数：圆 %g，正方形 %g\n", out[0], out[1]);

#if SHOW_ERROR == 3
    HostMade h;
    HostMade* d;
    int* r;
    CHECK(cudaMalloc(&d, sizeof h));
    CHECK(cudaMallocManaged(&r, sizeof(int)));
    CHECK(cudaMemcpy(d, &h, sizeof h, cudaMemcpyHostToDevice));   // 虚函数表指针指向主机上的地址
    bad<<<1, 1>>>(d, r);
    printf("调用主机上构造的对象的虚函数：%s\n", cudaGetErrorName(cudaDeviceSynchronize()));
#endif
}
