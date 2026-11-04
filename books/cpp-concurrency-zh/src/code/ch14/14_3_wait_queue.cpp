// mode: run
// expect: 等待队列出队 10000，校验和 50005000
// 两把锁的队列加条件变量：检查谓词时持有 head_mutex，wait 睡眠时释放；get_tail() 短暂锁 tail_mutex。
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

namespace {

template <class T>
class WaitQueue {
    struct Node {
        std::optional<T> data;
        std::unique_ptr<Node> next;
    };

public:
    WaitQueue() : head_(std::make_unique<Node>()), tail_(head_.get()) {}

    void push(T value) {
        auto new_node = std::make_unique<Node>();
        Node* const new_tail = new_node.get();
        {
            std::lock_guard<std::mutex> tail_lock(tail_mutex_);
            tail_->data = std::move(value);
            tail_->next = std::move(new_node);
            tail_ = new_tail;
        }
        data_ready_.notify_one();  // 解锁之后通知
    }

    // ANCHOR: wait
    std::optional<T> wait_pop() {
        std::unique_lock<std::mutex> head_lock(head_mutex_);
        data_ready_.wait(head_lock, [&] { return head_.get() != get_tail() || closed_; });
        if (head_.get() == get_tail()) {
            return std::nullopt;  // 已关闭且取空
        }
        std::unique_ptr<Node> old_head = std::move(head_);
        head_ = std::move(old_head->next);
        head_lock.unlock();
        return std::move(old_head->data);
    }
    // ANCHOR_END: wait

    void close() {
        {
            std::lock_guard<std::mutex> head_lock(head_mutex_);
            closed_ = true;
        }
        data_ready_.notify_all();
    }

private:
    Node* get_tail() {
        std::lock_guard<std::mutex> tail_lock(tail_mutex_);
        return tail_;
    }

    std::mutex head_mutex_;
    std::mutex tail_mutex_;
    std::condition_variable data_ready_;
    std::unique_ptr<Node> head_;
    Node* tail_;
    bool closed_ = false;
};

}  // namespace

int main() {
    constexpr int kItems = 10000;
    WaitQueue<int> queue;
    std::atomic<int> popped{0};
    std::atomic<std::uint64_t> checksum{0};

    std::thread producer([&] {
        for (int i = 1; i <= kItems; ++i) {
            queue.push(i);
        }
    });

    std::vector<std::thread> consumers;
    for (int c = 0; c < 3; ++c) {
        consumers.emplace_back([&] {
            std::uint64_t sum = 0;
            int n = 0;
            while (auto value = queue.wait_pop()) {
                sum += static_cast<std::uint64_t>(*value);
                ++n;
            }
            checksum.fetch_add(sum, std::memory_order_relaxed);
            popped.fetch_add(n, std::memory_order_relaxed);
        });
    }

    producer.join();
    queue.close();
    for (auto& thread : consumers) {
        thread.join();
    }

    std::printf("等待队列出队 %d，校验和 %llu\n", popped.load(),
                static_cast<unsigned long long>(checksum.load()));
    return 0;
}
