// mode: run
// 同一个进程里的线程：PID 相同、内核 TID 不同，栈也各自独立。
// 为了看清 ps -L 和 /proc/self/task，两个工作线程会多活一会儿。
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <dirent.h>
#include <thread>

namespace {

struct ThreadInfo {
    long tid;
    pthread_t self;
    uintptr_t stack_address;
};

long kernel_tid() {
    return static_cast<long>(::syscall(SYS_gettid));
}

int task_entries() {
    // /proc/self/task 下的每个目录项对应一个内核 TID。
    DIR* dir = ::opendir("/proc/self/task");
    if (dir == nullptr) {
        return -1;
    }
    int count = 0;
    while (const dirent* entry = ::readdir(dir)) {
        if (entry->d_name[0] != '.') {
            ++count;
        }
    }
    ::closedir(dir);
    return count;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "用法: %s <pid 文件>\n", argv[0]);
        return 2;
    }

    const long pid = static_cast<long>(::getpid());
    std::printf("主线程：getpid() == gettid()：%s\n", pid == kernel_tid() ? "是" : "否");

    constexpr int kThreads = 2;
    ThreadInfo info[kThreads]{};
    std::thread workers[kThreads];

    for (int i = 0; i < kThreads; ++i) {
        workers[i] = std::thread([i, &info] {
            char local = 0;
            info[i].tid = kernel_tid();
            info[i].self = ::pthread_self();
            info[i].stack_address = reinterpret_cast<uintptr_t>(&local);
            // 给 example.sh 留出运行 ps -L 和读 /proc 的时间。
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        });
    }

    FILE* file = std::fopen(argv[1], "w");
    if (file == nullptr) {
        return 3;
    }
    std::fprintf(file, "%ld\n", pid);
    std::fclose(file);

    std::printf("/proc/self/task 的目录项数：%d\n", task_entries());
    std::printf("两个工作线程的 pthread_self 不同：%s\n",
                ::pthread_equal(info[0].self, info[1].self) == 0 ? "是" : "否");

    for (auto& worker : workers) {
        worker.join();
    }

    const uintptr_t a = info[0].stack_address;
    const uintptr_t b = info[1].stack_address;
    const uintptr_t distance = a > b ? a - b : b - a;
    std::printf("两个工作线程：gettid 不同：%s\n", info[0].tid != info[1].tid ? "是" : "否");
    std::printf("两个工作线程：gettid 都不等于 PID：%s\n",
                (info[0].tid != pid && info[1].tid != pid) ? "是" : "否");
    std::printf("两个线程栈上局部变量的地址差 > 1 MiB：%s\n",
                distance > (1U << 20) ? "是" : "否");
    std::printf("地址差落在 1–64 MiB：%s\n",
                (distance > (1U << 20) && distance < (64U << 20)) ? "是" : "否");
    return 0;
}
