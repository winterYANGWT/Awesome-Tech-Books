// mode: run
// expect: 进程：地址相同 = true，值相同 = false
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <thread>

int shared_value = 10;

struct ChildReport {
    int value;
    std::uintptr_t address;
};

int main() {
    int fds[2];
    if (pipe(fds) != 0) {
        std::perror("pipe");
        return 1;
    }

    const pid_t pid = fork();
    if (pid < 0) {
        std::perror("fork");
        return 1;
    }
    if (pid == 0) {
        close(fds[0]);
        shared_value = 20;
        ChildReport report{shared_value,
                           reinterpret_cast<std::uintptr_t>(&shared_value)};
        const ssize_t n = write(fds[1], &report, sizeof report);
        close(fds[1]);
        // 子进程用 _exit：不冲刷父进程继承来的 stdio 缓冲，也不跑 atexit。
        _exit(n == static_cast<ssize_t>(sizeof report) ? 0 : 1);
    }

    close(fds[1]);
    ChildReport child{};
    const ssize_t n = read(fds[0], &child, sizeof child);
    close(fds[0]);
    int status = 0;
    waitpid(pid, &status, 0);

    if (n != static_cast<ssize_t>(sizeof child)) {
        std::cerr << "读子进程报告失败\n";
        return 1;
    }

    const auto parent_address = reinterpret_cast<std::uintptr_t>(&shared_value);
    std::cout << "进程：子进程把值改成 " << child.value
              << "；父进程读到 " << shared_value << '\n';
    std::cout << "进程：地址相同 = " << std::boolalpha
              << (child.address == parent_address)
              << "，值相同 = " << (child.value == shared_value) << '\n';

    // 线程版：同一个全局对象，工作线程改完，主线程 join 后能读到。
    std::uintptr_t thread_address = 0;
    int thread_value = 0;
    std::thread t([&] {
        shared_value = 30;
        thread_value = shared_value;
        thread_address = reinterpret_cast<std::uintptr_t>(&shared_value);
    });
    t.join();

    std::cout << "线程：工作线程把值改成 " << thread_value
              << "；主线程读到 " << shared_value << '\n';
    std::cout << "线程：地址相同 = " << (thread_address == parent_address)
              << "，值相同 = " << (thread_value == shared_value) << '\n';
}
