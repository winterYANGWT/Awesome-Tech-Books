// mode: run
// output: varies
// expect: hardware_concurrency=[0-9]+
// expect: affinity_cpus=[0-9]+
// expect: cpu.max=
// expect: pids.max=

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>
#include <sys/resource.h>
#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <thread>

static int affinity_count() {
    cpu_set_t set;
    CPU_ZERO(&set);
    if (sched_getaffinity(0, sizeof(set), &set) != 0) return -1;
    return CPU_COUNT(&set);
}

static std::string cgroup_v2_path() {
    std::ifstream in("/proc/self/cgroup");
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("0::", 0) == 0) return line.substr(3);
    }
    return {};
}

static std::string read_token(const std::string& path) {
    std::ifstream in(path);
    std::string value;
    if (!(in >> value)) return "unavailable";
    return value;
}

int main() {
    const std::string cg = cgroup_v2_path();
    const std::string root = "/sys/fs/cgroup" + (cg == "/" ? "" : cg);

    rlimit nproc_limit{};
    getrlimit(RLIMIT_NPROC, &nproc_limit);

    std::ifstream cpu_max(root + "/cpu.max");
    std::string quota;
    std::string period;
    if (!(cpu_max >> quota >> period)) {
        quota = "unavailable";
        period.clear();
    }

    std::printf("hardware_concurrency=%u\n", std::thread::hardware_concurrency());
    std::printf("affinity_cpus=%d\n", affinity_count());
    std::printf("cgroup=%s\n", cg.empty() ? "unavailable" : cg.c_str());
    std::printf("cpu.max=%s %s\n", quota.c_str(), period.c_str());
    std::printf("pids.max=%s\n", read_token(root + "/pids.max").c_str());
    std::printf("rlimit_nproc=%llu\n", static_cast<unsigned long long>(nproc_limit.rlim_cur));
    std::printf("threads_max=%s\n", read_token("/proc/sys/kernel/threads-max").c_str());
}
