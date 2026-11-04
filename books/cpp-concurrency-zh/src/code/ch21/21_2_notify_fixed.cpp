// mode: run
// expect: 完成 1 个，异常 1 个，消费者全部退出\n捕获到的异常：任务本身失败
// 队列存 shared_ptr<Item>，分配在 push() 里做；wait_pop() 只拷贝指针，
// 构造返回值不再抛异常。消费者处理任务时仍可能抛，必须自己 catch。
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

struct Item {
    int id;
    bool bad;

    Item(int i, bool b) : id(i), bad(b) {}

    void run() const {
        if (bad) {
            throw std::runtime_error("任务本身失败");
        }
    }
};

// ANCHOR: queue
class ItemQueue {
public:
    void push(Item value) {
        // 可能抛的分配放在入队之前；失败时队列保持原样。
        auto ptr = std::make_shared<Item>(std::move(value));
        {
            std::lock_guard<std::mutex> lock(m_);
            if (closed_) {
                return;
            }
            q_.push(std::move(ptr));
        }
        cv_.notify_one();
    }

// ANCHOR: pop
    std::shared_ptr<Item> wait_pop() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&] { return !q_.empty() || closed_; });
        if (q_.empty()) {
            return nullptr;
        }
        std::shared_ptr<Item> ptr = std::move(q_.front());
        q_.pop();
        return ptr;  // 拷贝 shared_ptr 不抛
    }
// ANCHOR_END: pop

    void close() {
        {
            std::lock_guard<std::mutex> lock(m_);
            closed_ = true;
        }
        cv_.notify_all();
    }

private:
    std::mutex m_;
    std::condition_variable cv_;
    std::queue<std::shared_ptr<Item>> q_;
    bool closed_ = false;
};
// ANCHOR_END: queue

}  // namespace

int main() {
    ItemQueue q;
    std::vector<std::thread> consumers;
    std::atomic<int> completed{0};
    std::atomic<int> failed{0};
    std::mutex error_mutex;
    std::string error_text;

    for (int i = 0; i < 2; ++i) {
        consumers.emplace_back([&] {
            while (auto item = q.wait_pop()) {
                try {
                    item->run();
                    completed.fetch_add(1, std::memory_order_relaxed);
                } catch (const std::exception& e) {
                    failed.fetch_add(1, std::memory_order_relaxed);
                    std::lock_guard<std::mutex> lock(error_mutex);
                    error_text = e.what();
                }
            }
        });
    }

    q.push(Item{1, true});   // 这个任务会抛
    q.push(Item{2, false});  // 这个任务正常
    q.close();

    for (std::thread& t : consumers) {
        t.join();
    }

    std::printf("完成 %d 个，异常 %d 个，消费者全部退出\n", completed.load(),
                failed.load());
    std::printf("捕获到的异常：%s\n", error_text.c_str());
    return 0;
}
