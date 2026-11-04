// mode: hang
// flags: -fno-inline
// 关掉内联，让 gdb 栈保留函数帧。
// expect: pthread_cond_wait|仍未结束
// 复刻旧教程里的 cond 封装：互斥量藏在类里，wait() 只调用一次 pthread_cond_wait。
// 生产者先 signal，消费者随后才进入 wait；条件变量没有记忆，通知丢失。
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <thread>

namespace {

void die(const char* what, int rc) {
    std::fprintf(stderr, "%s: %s\n", what, std::strerror(rc));
    std::exit(1);
}

// ANCHOR: cond
class Cond {
public:
    Cond() {
        int rc = pthread_mutex_init(&m_, nullptr);
        if (rc != 0) die("pthread_mutex_init", rc);
        rc = pthread_cond_init(&c_, nullptr);
        if (rc != 0) die("pthread_cond_init", rc);
    }
    ~Cond() {
        pthread_cond_destroy(&c_);
        pthread_mutex_destroy(&m_);
    }

    // 旧写法：没有谓词循环，也没有把互斥量交给调用者。
    void wait() {
        int rc = pthread_mutex_lock(&m_);
        if (rc != 0) die("pthread_mutex_lock", rc);
        rc = pthread_cond_wait(&c_, &m_);
        if (rc != 0) die("pthread_cond_wait", rc);
        rc = pthread_mutex_unlock(&m_);
        if (rc != 0) die("pthread_mutex_unlock", rc);
    }

    void signal() {
        int rc = pthread_cond_signal(&c_);
        if (rc != 0) die("pthread_cond_signal", rc);
    }

private:
    pthread_mutex_t m_;
    pthread_cond_t c_;
};
// ANCHOR_END: cond

}  // namespace

int main() {
    Cond cond;
    std::thread consumer([&] {
        // 为了稳定演示而强制的交错：让生产者先 signal，再进入 wait。
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::printf("消费者进入 wait\n");
        std::fflush(stdout);
        cond.wait();
        std::printf("消费者被唤醒\n");
    });

    std::printf("生产者 signal（此时没有等待者）\n");
    std::fflush(stdout);
    cond.signal();
    consumer.join();
    std::printf("不会走到这里\n");
    return 0;
}
