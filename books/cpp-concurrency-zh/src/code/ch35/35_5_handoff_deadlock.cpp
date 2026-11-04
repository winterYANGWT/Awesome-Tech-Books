// 推选新 Leader 时忘记唤醒：4 个线程最后都睡在条件变量上。
// mode: hang
// timeout: 2
// expect: 仍未结束
#include <sys/epoll.h>
#include <unistd.h>

#include <barrier>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

namespace {

constexpr int kThreads = 4;

std::mutex mu;
std::condition_variable cv;
bool stop = false;  // 没有人会把它置真：等唤醒的一方永远等不到
int leader = 0;
int waiting = 0;  // 已经睡到 cv 上的 follower 数
int epfd = -1;
int pipe_read = -1;
std::barrier sync_point(kThreads);

void worker(int id) {
    // 先让 4 个线程都就位，再由线程 0 去取事件。
    sync_point.arrive_and_wait();

    if (id != 0) {
        std::unique_lock<std::mutex> lock(mu);
        ++waiting;
        cv.notify_all();  // 叫醒线程 0 的“等 follower 就位”，与下面的漏通知无关
        cv.wait(lock, [id] { return stop || leader == id; });
        return;
    }

    epoll_event event{};
    const int n = epoll_wait(epfd, &event, 1, 1000);
    if (n > 0) {
        char byte = 0;
        const ssize_t got = read(pipe_read, &byte, 1);
        (void)got;
        {
            std::unique_lock<std::mutex> guard(mu);
            // 等三个 follower 都睡下再改 leader：否则检查得晚的那个会直接看到新值。
            cv.wait(guard, [] { return waiting == kThreads - 1; });
            leader = 1;  // 推选了 follower 1，却没有 cv.notify_all()
        }
        std::printf("leader 0: 收到事件，推选 follower 1（忘记唤醒）\n");
        std::printf("leader 0: 处理完，回到 follower\n");
        std::fflush(stdout);
    }

    // leader == 1 的那个 follower 条件已经满足，但没有通知，它不会醒来；
    // 线程 0 也在等一个永远不会到来的通知。
    std::unique_lock<std::mutex> lock(mu);
    cv.wait(lock, [id] { return stop || leader == id; });
}

}  // namespace

int main() {
    int pipe_fds[2];
    if (pipe(pipe_fds) != 0) return 1;
    pipe_read = pipe_fds[0];
    const char byte = 'x';
    const ssize_t written = write(pipe_fds[1], &byte, 1);
    if (written != 1) return 1;

    epfd = epoll_create1(EPOLL_CLOEXEC);
    epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = pipe_read;
    epoll_ctl(epfd, EPOLL_CTL_ADD, pipe_read, &event);

    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) threads.emplace_back(worker, i);

    // 主线程也阻塞在 join 上；2 秒后由检查脚本取线程栈。
    for (auto &t : threads) t.join();

    close(pipe_fds[0]);
    close(pipe_fds[1]);
    close(epfd);
}
