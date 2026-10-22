// repeat_sort.cu: 同一个大小的排序反复做很多次。Thrust 每次调用都临时分配内存；
// CUB 可以预先分配好临时空间反复使用。
//   nvcc -O2 -arch=native repeat_sort.cu -o repeat_sort && ./repeat_sort
#include <cub/cub.cuh>
#include <thrust/device_vector.h>
#include <thrust/sort.h>
#include <chrono>
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

double now_us() {
    using namespace std::chrono;
    return duration<double, std::micro>(steady_clock::now().time_since_epoch()).count();
}

int main() {
    const int reps = 100;
    printf("     元素个数    thrust::sort    CUB（预分配临时空间）   CUB 需要的临时空间\n");
    for (size_t n : {size_t(1) << 16, size_t(1) << 20, size_t(1) << 24}) {
        std::vector<unsigned> h(n);
        std::mt19937 rng(3);
        for (auto& v : h) v = rng();
        thrust::device_vector<unsigned> src(h.begin(), h.end()), work(n), out(n);

        // Thrust：每次调用内部都要申请、释放一块临时内存。
        work = src;
        thrust::sort(work.begin(), work.end());               // 预热
        CHECK(cudaDeviceSynchronize());
        double t0 = now_us();
        for (int r = 0; r < reps; ++r) {
            CHECK(cudaMemcpyAsync(thrust::raw_pointer_cast(work.data()), thrust::raw_pointer_cast(src.data()),
                                  n * sizeof(unsigned), cudaMemcpyDeviceToDevice));
            thrust::sort(work.begin(), work.end());
        }
        CHECK(cudaDeviceSynchronize());
        double t_thrust = (now_us() - t0) / reps;

        // ANCHOR: cub
        // CUB：第一次调用只问临时空间要多大，分配一次，以后反复用。
        const unsigned* in = thrust::raw_pointer_cast(src.data());
        unsigned* o = thrust::raw_pointer_cast(out.data());
        void* temp = nullptr;
        size_t temp_bytes = 0;
        CHECK(cub::DeviceRadixSort::SortKeys(temp, temp_bytes, in, o, int(n)));
        CHECK(cudaMalloc(&temp, temp_bytes));
        CHECK(cub::DeviceRadixSort::SortKeys(temp, temp_bytes, in, o, int(n)));   // 预热
        CHECK(cudaDeviceSynchronize());
        t0 = now_us();
        for (int r = 0; r < reps; ++r) {        // 和上面一样，每轮也做一次同样大小的拷贝，比较才公平
            CHECK(cudaMemcpyAsync(thrust::raw_pointer_cast(work.data()), in, n * sizeof(unsigned),
                                  cudaMemcpyDeviceToDevice));
            CHECK(cub::DeviceRadixSort::SortKeys(temp, temp_bytes, in, o, int(n)));
        }
        CHECK(cudaDeviceSynchronize());
        double t_cub = (now_us() - t0) / reps;
        // ANCHOR_END: cub
        CHECK(cudaFree(temp));
        thrust::sort(work.begin(), work.end());               // work 现在是未排序的拷贝，排一下用来核对
        bool same = thrust::equal(out.begin(), out.end(), work.begin());
        printf("%13zu  %10.1f us  %16.1f us  %16.1f MB   %s\n", n, t_thrust, t_cub, temp_bytes / 1048576.0,
               same ? "结果一致" : "结果不一致");
    }
}
