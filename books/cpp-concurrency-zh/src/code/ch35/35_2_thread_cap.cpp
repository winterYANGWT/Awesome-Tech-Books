// 每连接一个线程的上限：第 257 条连接时按 pthread_create 的 EAGAIN 处理。
// mode: crash
// expect: 第 257 条连接
#include <pthread.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

constexpr int kMaxThreads = 256;

void *serve_one(void *) {
    // 真实服务器在这里读请求；为了只演示线程数量，让它一直等。
    sleep(60);
    return nullptr;
}

}  // namespace

int main() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    for (int conn = 1;; ++conn) {
        if (conn > kMaxThreads) {
            std::fprintf(stderr,
                         "第 %d 条连接：线程数已达到 %d，pthread_create 返回 "
                         "EAGAIN（模拟）\n",
                         conn, kMaxThreads);
            assert(false && "pthread_create: EAGAIN");
        }
        pthread_t tid;
        const int rc = pthread_create(&tid, &attr, serve_one, nullptr);
        if (rc != 0) {
            // pthread_create 返回错误码，不设置 errno。
            std::fprintf(stderr, "第 %d 条连接：pthread_create: %s\n", conn,
                         std::strerror(rc));
            std::abort();
        }
        if (conn == 1 || conn % 64 == 0) {
            std::printf("已为 %d 条连接创建线程\n", conn);
        }
    }

    pthread_attr_destroy(&attr);
    return 0;
}
