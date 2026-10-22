// thrust_basics.cu: 用 Thrust 做几种常见的并行操作：排序、归约、扫描、流压缩、分组统计，并和 CPU 单线程比较。
//   nvcc -O2 -arch=native --extended-lambda thrust_basics.cu -o thrust_basics && ./thrust_basics
// NVCCFLAGS: --extended-lambda
#include <thrust/copy.h>
#include <thrust/count.h>
#include <thrust/device_vector.h>
#include <thrust/functional.h>
#include <thrust/host_vector.h>
#include <thrust/iterator/constant_iterator.h>
#include <thrust/reduce.h>
#include <thrust/scan.h>
#include <thrust/sort.h>
#include <thrust/transform_reduce.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <numeric>
#include <random>
#include <vector>

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

// GPU 计时：执行 f，等 GPU 做完，返回毫秒数。
template <class F>
double gpu_ms(F f) {
    cudaDeviceSynchronize();
    double t0 = now_ms();
    f();
    cudaDeviceSynchronize();
    return now_ms() - t0;
}

// ANCHOR: functors
struct square {
    __host__ __device__ float operator()(float v) const { return v * v; }
};
struct is_positive {
    __host__ __device__ bool operator()(float v) const { return v > 0.0f; }
};
// ANCHOR_END: functors

int main() {
    const size_t n = size_t(32) << 20;           // 3200 多万个元素
    std::mt19937 rng(7);
    std::vector<unsigned> h_keys(n);
    std::vector<float> h_vals(n);
    for (size_t i = 0; i < n; ++i) h_keys[i] = rng(), h_vals[i] = float(int(rng() % 2001) - 1000) / 1000.0f;

    // ANCHOR: vectors
    thrust::device_vector<unsigned> keys(h_keys.begin(), h_keys.end());   // 分配显存并拷过去
    thrust::device_vector<float> vals(h_vals.begin(), h_vals.end());
    // ANCHOR_END: vectors

    printf("%zu 个元素           GPU（Thrust）   CPU 单线程\n", n);
    // ANCHOR: sort
    thrust::device_vector<unsigned> k1 = keys;
    double g_sort = gpu_ms([&] { thrust::sort(k1.begin(), k1.end()); });
    // ANCHOR_END: sort
    std::vector<unsigned> c1 = h_keys;
    double t0 = now_ms();
    std::sort(c1.begin(), c1.end());
    double c_sort = now_ms() - t0;
    bool sorted_ok = thrust::equal(k1.begin(), k1.end(), c1.begin());
    printf("排序 unsigned       %8.1f ms   %8.1f ms   %s\n", g_sort, c_sort, sorted_ok ? "结果一致" : "结果不一致");

    // ANCHOR: reduce
    float g_ss = 0;
    double g_red = gpu_ms([&] {
        g_ss = thrust::transform_reduce(vals.begin(), vals.end(), square(), 0.0f, thrust::plus<float>());
    });
    // ANCHOR_END: reduce
    t0 = now_ms();
    double c_ss = 0;
    for (float v : h_vals) c_ss += double(v) * v;
    double c_red = now_ms() - t0;
    printf("平方和              %8.2f ms   %8.1f ms   相对差 %.1e\n", g_red, c_red, std::abs(g_ss - c_ss) / c_ss);

    // ANCHOR: scan
    thrust::device_vector<float> prefix(n);
    double g_scan = gpu_ms([&] { thrust::inclusive_scan(vals.begin(), vals.end(), prefix.begin()); });
    // ANCHOR_END: scan
    std::vector<float> c_prefix(n);
    t0 = now_ms();
    std::partial_sum(h_vals.begin(), h_vals.end(), c_prefix.begin());
    double c_scan = now_ms() - t0;
    printf("前缀和              %8.2f ms   %8.1f ms\n", g_scan, c_scan);

    // ANCHOR: compact
    // 流压缩：只保留正数。先数出有多少个，分配好，再拷。
    thrust::device_vector<float> positives;
    double g_compact = gpu_ms([&] {
        size_t m = thrust::count_if(vals.begin(), vals.end(), is_positive());
        positives.resize(m);
        thrust::copy_if(vals.begin(), vals.end(), positives.begin(), is_positive());
    });
    // ANCHOR_END: compact
    t0 = now_ms();
    std::vector<float> c_pos;
    c_pos.reserve(n);
    std::copy_if(h_vals.begin(), h_vals.end(), std::back_inserter(c_pos), [](float v) { return v > 0.0f; });
    double c_compact = now_ms() - t0;
    printf("只保留正数          %8.2f ms   %8.1f ms   留下 %zu / %zu 个\n", g_compact, c_compact, positives.size(),
           c_pos.size());

    // ANCHOR: groupby
    // 分组统计：每个元素属于 keys % 1000 这一组，数出每组有多少个、组内的值加起来是多少。
    thrust::device_vector<unsigned> group(n), group_out(1000);
    thrust::device_vector<float> v2 = vals, sum_out(1000);
    thrust::device_vector<int> count_out(1000);
    double g_group = gpu_ms([&] {
        thrust::transform(keys.begin(), keys.end(), group.begin(),
                          [] __device__(unsigned k) { return k % 1000; });
        thrust::sort_by_key(group.begin(), group.end(), v2.begin());       // 按组号排序，值跟着走
        thrust::reduce_by_key(group.begin(), group.end(), v2.begin(), group_out.begin(), sum_out.begin());
        thrust::reduce_by_key(group.begin(), group.end(), thrust::constant_iterator<int>(1),
                              group_out.begin(), count_out.begin());
    });
    // ANCHOR_END: groupby
    printf("按 1000 组统计      %8.2f ms   第 0 组：%d 个元素，和 %.2f\n", g_group, int(count_out[0]),
           float(sum_out[0]));
}
