// Absolute-time sleep and SCHED_FIFO probe for a 2 ms control loop (10.3).
//
// The program asks for SCHED_FIFO, reports whether the kernel granted it, and
// then restores SCHED_OTHER so a 10 s benchmark cannot starve the machine.
// Each cycle does about 0.12 ms of simulated work and sleeps until the absolute
// deadline t0 + k*2 ms with clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME).
// Jitter is the wake-up time minus that deadline.
//
// Build:
//   clang++ -std=c++20 -O2 -pthread 10_3_realtime_cpp.cpp -o 10_3_realtime_cpp

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sched.h>
#include <unistd.h>
#include <vector>

namespace {

constexpr long long kPeriodNs = 2000000LL;   // 2 ms
constexpr long long kWorkNs = 120000LL;      // simulated 0.12 ms of work

long long diff_ns(const timespec &a, const timespec &b) {
    return static_cast<long long>(a.tv_sec - b.tv_sec) * 1000000000LL +
           static_cast<long long>(a.tv_nsec - b.tv_nsec);
}

void add_ns(timespec &t, long long ns) {
    t.tv_sec += static_cast<time_t>(ns / 1000000000LL);
    t.tv_nsec += static_cast<long>(ns % 1000000000LL);
    if (t.tv_nsec >= 1000000000L) {
        t.tv_nsec -= 1000000000L;
        t.tv_sec += 1;
    }
}

void work(long long ns) {
    timespec a, b;
    clock_gettime(CLOCK_MONOTONIC, &a);
    do {
        clock_gettime(CLOCK_MONOTONIC, &b);
    } while (diff_ns(b, a) < ns);
}

}  // namespace

// ANCHOR: main
int main() {
    constexpr int N = 5000;
    std::printf("=== 10.3 C++ 绝对时间睡眠的抖动（%d 个周期） ===\n", N);

    sched_param sp{};
    const int prio = sched_get_priority_max(SCHED_FIFO);
    sp.sched_priority = prio;
    const bool ok = sched_setscheduler(0, SCHED_FIFO, &sp) == 0;
    const int err = errno;
    std::printf("SCHED_FIFO 最高优先级 = %d；设置结果：%s", prio,
                ok ? "成功" : "失败");
    if (!ok) {
        std::printf("（errno=%d: %s，需要 root 或 CAP_SYS_NICE）", err,
                    std::strerror(err));
    } else {
        sched_param sp0{};
        sched_setscheduler(0, SCHED_OTHER, &sp0);
        std::printf("（已恢复普通策略，避免占满 CPU）");
    }
    std::printf("\n");

    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    if (sched_setaffinity(0, sizeof(set), &set) == 0) {
        std::printf("CPU 亲和性：已绑定 CPU 0\n");
    } else {
        std::printf("CPU 亲和性：未绑定（errno=%d）\n", errno);
    }

    timespec t0{};
    clock_gettime(CLOCK_MONOTONIC, &t0);
    std::vector<double> jitter;
    jitter.reserve(N);
    for (int k = 0; k < N; ++k) {
        timespec target = t0;
        add_ns(target, static_cast<long long>(k) * kPeriodNs);
        work(kWorkNs);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &target, nullptr);
        timespec now{};
        clock_gettime(CLOCK_MONOTONIC, &now);
        jitter.push_back(static_cast<double>(diff_ns(now, target)) / 1e6);
    }

    std::vector<double> sorted = jitter;
    std::sort(sorted.begin(), sorted.end());
    double sum = 0.0;
    for (double v : jitter) sum += v;
    const double mean = sum / N;
    const double p95 = sorted[static_cast<std::size_t>(0.95 * (N - 1))];
    const double mx = sorted.back();
    const double last = jitter.back();

    std::printf("\n策略                       均值/ms   95%%/ms   最大/ms   末周期偏差/ms\n");
    std::printf("clock_nanosleep 绝对时间 %8.4f %8.4f %8.4f %12.4f\n",
                mean, p95, mx, last);
    std::printf("说明：与 Python 版“忙等 + 绝对时间”使用同一个 2 ms 目标；"
                "C++ 少了解释器开销\n");
    return 0;
}
// ANCHOR_END: main
