// mode: run
// expect: hardware_concurrency: \d+\nsched_getaffinity: \d+\ncgroup cpu.max: .+\n建议池大小: \d+
// 三个数可能不同：标准库的提示、进程的 CPU 亲和性、cgroup 的配额。
#ifndef _GNU_SOURCE
#define _GNU_SOURCE  // sched_getaffinity 需要
#endif
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sched.h>
#include <string>
#include <thread>

namespace {

long affinity_cpus() {
    cpu_set_t set;
    CPU_ZERO(&set);
    if (sched_getaffinity(0, sizeof(set), &set) == -1) {
        return -1;
    }
    return CPU_COUNT(&set);
}

// cgroup v2 的 cpu.max 是 "quota period"；第一列为 max 表示不限。
int cgroup_quota_cpus(std::string& raw) {
    std::ifstream in("/sys/fs/cgroup/cpu.max");
    std::string quota;
    std::string period;
    if (!(in >> quota >> period)) {
        raw = "不可读";
        return -1;
    }
    raw = quota + " " + period;
    if (quota == "max") {
        return -1;
    }
    const long q = std::stol(quota);
    const long p = std::stol(period);
    if (q <= 0 || p <= 0) {
        return -1;
    }
    return static_cast<int>((q + p - 1) / p);
}

}  // namespace

int main() {
    const unsigned hw = std::thread::hardware_concurrency();
    const long affinity = affinity_cpus();
    std::string cgroup_raw;
    const int quota = cgroup_quota_cpus(cgroup_raw);

    std::printf("hardware_concurrency: %u\n", hw);
    std::printf("sched_getaffinity: %ld\n", affinity);
    std::printf("cgroup cpu.max: %s\n", cgroup_raw.c_str());

    unsigned pool = hw == 0 ? 1 : hw;
    if (affinity > 0) {
        pool = std::min(pool, static_cast<unsigned>(affinity));
    }
    if (quota > 0) {
        pool = std::min(pool, static_cast<unsigned>(quota));
    }
    std::printf("建议池大小: %u（CPU 密集型起点，I/O 密集型要另算）\n", pool);
    return 0;
}
