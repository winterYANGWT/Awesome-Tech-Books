// mode: run
// output: varies
// expect: 同一 PID 下
#include <dirent.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

std::atomic<bool> release{false};

long kernel_tid() {
    return static_cast<long>(syscall(SYS_gettid));
}

void worker(int id) {
    std::printf("worker %d: gettid=%ld pthread_self=%llu\n", id, kernel_tid(),
                static_cast<unsigned long long>(pthread_self()));
    while (!release.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main() {
    std::printf("进程 PID=%ld\n", static_cast<long>(getpid()));
    std::printf("主线程 gettid=%ld pthread_self=%llu\n", kernel_tid(),
                static_cast<unsigned long long>(pthread_self()));

    std::vector<std::thread> threads;
    for (int i = 0; i < 3; ++i) threads.emplace_back(worker, i);
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));

    std::printf("同一 PID 下，每个线程有自己的 TID：");
    if (DIR* dir = opendir("/proc/self/task")) {
        while (dirent* entry = readdir(dir)) {
            if (entry->d_name[0] == '.') continue;
            std::printf("%s ", entry->d_name);
        }
        closedir(dir);
    }
    std::printf("\n");

    release.store(true, std::memory_order_relaxed);
    for (auto& t : threads) t.join();
}
