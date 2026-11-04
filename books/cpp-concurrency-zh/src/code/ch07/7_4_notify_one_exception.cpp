// mode: hang
// flags: -fno-inline
// 关掉内联，让 gdb 栈保留函数帧。
// expect: pthread_cond_wait|仍未结束
// 消费者 A 取走唯一的任务，在把任务移进返回值时抛异常；元素已经不在队列里。
// notify_one 只唤醒了 A，B 还在等；生产者已经结束，主线程 join B 时挂住。
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <latch>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>

namespace {

struct Job {
    int id;
    bool armed;

    Job(int i, bool a) : id(i), armed(a) {}
    Job(const Job&) = delete;
    Job& operator=(const Job&) = delete;

    Job(Job&& other) : id(other.id), armed(other.armed) {
        if (armed) {
            armed = false;
            throw std::runtime_error("构造返回值时移动失败");
        }
    }
};

// 队列内部存 unique_ptr<Job>，wait_pop 把 Job 移进 optional 返回。
// 元素先 pop 再构造返回值：构造抛异常时，任务已经不在队列里。
class JobQueue {
public:
    void push(std::unique_ptr<Job> value) {
        {
            std::lock_guard<std::mutex> lock(m_);
            q_.push(std::move(value));
        }
        cv_.notify_one();
    }

// ANCHOR: pop
    std::optional<Job> wait_pop() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&] { return !q_.empty(); });
        auto ptr = std::move(q_.front());
        q_.pop();
        return std::optional<Job>(std::in_place, std::move(*ptr));  // 这里可能抛
    }
// ANCHOR_END: pop

private:
    std::mutex m_;
    std::condition_variable cv_;
    std::queue<std::unique_ptr<Job>> q_;
};

}  // namespace

int main() {
    JobQueue q;
    std::latch a_entered{1};

    std::thread a([&] {
        a_entered.count_down();
        try {
            auto job = q.wait_pop();
            std::printf("A 取到任务 %d\n", job ? job->id : -1);
        } catch (const std::exception& e) {
            std::printf("A 取到任务，构造返回值时抛异常：%s\n", e.what());
        }
        std::printf("A 退出\n");
    });

    a_entered.wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    q.push(std::make_unique<Job>(1, true));
    std::printf("生产者放入 1 个任务并 notify_one\n");
    std::fflush(stdout);
    a.join();

    std::thread b([&] {
        std::printf("B 进入 wait_pop（队列为空，生产者已结束）\n");
        std::fflush(stdout);
        auto job = q.wait_pop();  // 没有 close，永远等不到
        std::printf("B 取到任务 %d\n", job ? job->id : -1);
    });
    b.join();

    std::printf("不会走到这里\n");
    return 0;
}
