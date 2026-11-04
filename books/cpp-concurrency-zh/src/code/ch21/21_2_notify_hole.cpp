// mode: hang
// flags: -fno-inline
// expect: pthread_cond_wait|仍未结束
// 消费者 A 被 notify_one 唤醒，取走唯一的元素，解锁后构造返回值时抛异常。
// B 这时已经在 wait 上；通知已经被 A 消耗，生产者也不再 push，B 永远醒不来。
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

std::latch g_popped{1};
std::latch g_release{1};

// ANCHOR: item
struct Item {
    int id;
    bool armed;

    Item(int i, bool a) : id(i), armed(a) {}
    Item(const Item&) = delete;
    Item& operator=(const Item&) = delete;

    Item(Item&& other) : id(other.id), armed(other.armed) {
        if (armed) {
            std::printf("A 在构造返回值时进入移动构造\n");
            std::fflush(stdout);
            g_popped.count_down();
            g_release.wait();  // 等 B 也睡到 wait 上，再抛
            throw std::runtime_error("构造返回值时移动失败");
        }
    }
};

// ANCHOR_END: item

class ItemQueue {
public:
    void push(std::unique_ptr<Item> value) {
        {
            std::lock_guard<std::mutex> lock(m_);
            q_.push(std::move(value));
        }
        cv_.notify_one();
    }

// ANCHOR: pop
    std::optional<Item> wait_pop() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&] { return !q_.empty(); });
        std::unique_ptr<Item> ptr = std::move(q_.front());
        q_.pop();
        lock.unlock();  // 先摘除、解锁，再构造返回值
        return std::optional<Item>(std::in_place, std::move(*ptr));  // 这里可能抛
    }
// ANCHOR_END: pop

private:
    std::mutex m_;
    std::condition_variable cv_;
    std::queue<std::unique_ptr<Item>> q_;
};

}  // namespace

int main() {
    ItemQueue q;

    std::thread a([&] {
        std::printf("A 进入 wait_pop\n");
        std::fflush(stdout);
        try {
            auto item = q.wait_pop();
            std::printf("A 取到任务 %d\n", item->id);
        } catch (const std::exception& e) {
            std::printf("A 构造返回值抛异常：%s\n", e.what());
        }
        std::printf("A 退出\n");
        std::fflush(stdout);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    q.push(std::make_unique<Item>(1, true));
    std::printf("生产者放入唯一的任务并 notify_one\n");
    std::fflush(stdout);

    g_popped.wait();  // A 已经把元素摘走，正卡在移动构造里
    std::thread b([&] {
        std::printf("B 进入 wait_pop（队列为空）\n");
        std::fflush(stdout);
        auto item = q.wait_pop();
        std::printf("B 取到任务 %d\n", item->id);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    g_release.count_down();  // 放 A 去抛异常
    a.join();
    b.join();  // B 永远等不到第二次通知

    std::printf("不会走到这里\n");
    return 0;
}
