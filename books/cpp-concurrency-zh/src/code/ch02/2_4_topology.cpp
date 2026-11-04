// mode: run
// output: varies
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sched.h>
#include <unistd.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

std::string read_line(const std::string& path) {
    std::ifstream in(path);
    std::string line;
    std::getline(in, line);
    return line;
}

std::string cpu_list(const cpu_set_t& set, int ncpu) {
    std::string out;
    int start = -1;
    for (int cpu = 0; cpu <= ncpu; ++cpu) {
        const bool on = cpu < ncpu && CPU_ISSET(cpu, &set);
        if (on && start < 0) {
            start = cpu;
        } else if (!on && start >= 0) {
            if (!out.empty()) {
                out += ',';
            }
            out += std::to_string(start);
            if (cpu - 1 != start) {
                out += '-' + std::to_string(cpu - 1);
            }
            start = -1;
        }
    }
    return out;
}

}  // namespace

// ANCHOR: report
int main() {
    const long online = sysconf(_SC_NPROCESSORS_ONLN);

    cpu_set_t set;
    CPU_ZERO(&set);
    const int rc = sched_getaffinity(0, sizeof(set), &set);
    const int allowed = rc == 0 ? CPU_COUNT(&set) : -1;

    std::printf("hardware_concurrency() = %u\n", std::thread::hardware_concurrency());
    std::printf("sched_getaffinity cpus = %d (%s)\n", allowed,
                rc == 0 ? cpu_list(set, static_cast<int>(online)).c_str() : "失败");
    {
        std::ifstream status("/proc/self/status");
        std::string line;
        while (std::getline(status, line)) {
            if (line.rfind("Cpus_allowed_list:", 0) == 0) {
                std::printf("%s\n", line.c_str());
            }
        }
    }

    for (int index : {0, 2, 3}) {
        const std::string base = "/sys/devices/system/cpu/cpu0/cache/index" +
                                 std::to_string(index);
        const std::string level = read_line(base + "/level");
        const std::string type = read_line(base + "/type");
        const std::string size = read_line(base + "/size");
        std::printf("cache L%s %s = %s\n", level.c_str(), type.c_str(), size.c_str());
    }
    std::printf("L1d cache line = %ld B\n", sysconf(_SC_LEVEL1_DCACHE_LINESIZE));

    std::set<std::pair<std::string, std::string>> cores;
    for (long cpu = 0; cpu < online; ++cpu) {
        const std::string base = "/sys/devices/system/cpu/cpu" + std::to_string(cpu) +
                                 "/topology/";
        cores.emplace(read_line(base + "physical_package_id"), read_line(base + "core_id"));
    }
    std::printf("physical cores = %zu\n", cores.size());

    long nodes = 0;
    for (const auto& entry : std::filesystem::directory_iterator("/sys/devices/system/node")) {
        if (entry.path().filename().string().rfind("node", 0) == 0) {
            ++nodes;
        }
    }

    std::set<std::string> l3_groups;
    for (long cpu = 0; cpu < online; ++cpu) {
        const std::string base = "/sys/devices/system/cpu/cpu" + std::to_string(cpu) +
                                 "/cache/index3/";
        l3_groups.insert(read_line(base + "shared_cpu_list"));
    }
    std::printf("L3 instances = %zu\n", l3_groups.size());
    std::printf("NUMA nodes = %ld (online=%s)\n", nodes,
                read_line("/sys/devices/system/node/online").c_str());
    return 0;
}
// ANCHOR_END: report
