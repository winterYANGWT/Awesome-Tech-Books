// mode: hang
// expect: 仍未结束
// timeout: 2
// 反例：工作线程全部 detach，析构只置 stop，既不 post 信号量也不 join。
#include <chrono>
#include <cstdio>
#include <functional>
#include <latch>
#include <mutex>
#include <pthread.h>
#include <queue>
#include <semaphore.h>
#include <thread>
#include <utility>

namespace {

class DetachedPool {
public:
    explicit DetachedPool(int n) {
        sem_init(&sem_, 0, 0);
        for (int i = 0; i < n; ++i) {
            pthread_t tid;
            pthread_create(&tid, nullptr, &DetachedPool::entry, this);
            pthread_detach(tid);  // 建完就不管：之后无法 join
        }
    }

    ~DetachedPool() {
        stop_ = true;
        // 缺陷：没有 sem_post 唤醒等待者，也没有 join
    }

    void submit(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(m_);
            q_.push(std::move(task));
        }
        sem_post(&sem_);
    }

private:
    static void* entry(void* arg) {
        static_cast<DetachedPool*>(arg)->run();
        return nullptr;
    }

    void run() {
        for (;;) {
            while (sem_wait(&sem_) == -1) {
                // 示例里不处理 EINTR；真实代码要判断 errno
            }
            if (stop_) {
                return;
            }
            std::function<void()> task;
            {
                std::lock_guard<std::mutex> lock(m_);
                if (q_.empty()) {
                    continue;
                }
                task = std::move(q_.front());
                q_.pop();
            }
            task();
        }
    }

    std::mutex m_;
    std::queue<std::function<void()>> q_;
    sem_t sem_;
    bool stop_ = false;
};

}  // namespace

int main() {
    {
        DetachedPool pool(4);
        std::latch done(4);
        for (int i = 0; i < 4; ++i) {
            pool.submit([&done] { done.count_down(); });
        }
        done.wait();
        std::printf("4 个任务完成，池对象即将析构\n");
    }
    std::printf("析构返回，4 个工作线程仍阻塞在 sem_wait\n");
    // 主线程还活着，gdb 才能同时看到它和工作线程；真实故障里进程会一直不退出。
    std::this_thread::sleep_for(std::chrono::seconds(30));
    return 0;
}
