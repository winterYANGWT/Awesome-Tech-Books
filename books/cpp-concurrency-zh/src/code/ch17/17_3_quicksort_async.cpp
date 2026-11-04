// mode: crash
// expect: 创建线程失败
// timeout: 20
#include <sys/resource.h>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <future>
#include <random>
#include <system_error>
#include <vector>

std::vector<int> a;
std::atomic<int> started{0};

int partition_range(int lo, int hi) {
    int mid = lo + (hi - lo) / 2;
    if (a[mid] < a[lo]) std::swap(a[mid], a[lo]);
    if (a[hi - 1] < a[lo]) std::swap(a[hi - 1], a[lo]);
    if (a[hi - 1] < a[mid]) std::swap(a[hi - 1], a[mid]);
    std::swap(a[mid], a[hi - 1]);
    int pivot = a[hi - 1];
    int i = lo;
    for (int j = lo; j < hi - 1; ++j)
        if (a[j] < pivot) std::swap(a[i++], a[j]);
    std::swap(a[i], a[hi - 1]);
    return i;
}

// 每一层递归都开两个 std::async：线程数随深度指数增长，没有上限。
void parallel_sort(int lo, int hi) {
    if (hi - lo < 2) return;
    int p = partition_range(lo, hi);
    int n = ++started;
    if (n % 64 == 0) {
        std::printf("已启动 %d 个异步任务\n", n);
        std::fflush(stdout);
    }
    auto left = std::async(std::launch::async, [=] { parallel_sort(lo, p); });
    auto right = std::async(std::launch::async, [=] { parallel_sort(p + 1, hi); });
    left.get();
    right.get();
}

int main() {
    // 让这个反例在几秒内稳定复现：把地址空间压到 512 MB，线程栈很快申请不出来。
    // 不压的话，真实的无限递归也会失败，只是要先创建几万个线程，等很久。
    rlimit rl{512ull << 20, 512ull << 20};
    setrlimit(RLIMIT_AS, &rl);

    constexpr int N = 1 << 20;
    a.resize(N);
    std::mt19937 rng(11);
    std::uniform_int_distribution<int> dist(0, 1'000'000);
    for (int& v : a) v = dist(rng);

    try {
        parallel_sort(0, N);
    } catch (const std::system_error& e) {
        std::printf("创建线程失败: %s\n", e.what());
        std::fflush(stdout);
        throw;   // 没有捕获，std::terminate
    }
    std::printf("排序完成，共启动 %d 个任务\n", started.load());
}
