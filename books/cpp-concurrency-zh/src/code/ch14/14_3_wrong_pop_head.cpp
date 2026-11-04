// mode: race
// expect: data race
// 错误版本在 head_mutex 之外读 head，再拿这个快照去和 tail 比较。
// TSan 报的是 head 上的数据竞争；锁内用新的 get_tail() 重查只是让程序别真的弹空。
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace {

template <class T>
class WrongQueue {
    struct Node {
        std::optional<T> data;
        std::unique_ptr<Node> next;
    };

public:
    WrongQueue() : head_(std::make_unique<Node>()), tail_(head_.get()) {}

    void push(T value) {
        auto new_node = std::make_unique<Node>();
        Node* const new_tail = new_node.get();
        std::lock_guard<std::mutex> tail_lock(tail_mutex_);
        tail_->data = std::move(value);
        tail_->next = std::move(new_node);
        tail_ = new_tail;
    }

    // ANCHOR: wrong
    std::optional<T> wrong_try_pop() {
        Node* const tail_snapshot = get_tail();
        if (head_.get() == tail_snapshot) {  // 锁外读 head：数据竞争
            return std::nullopt;
        }

        std::lock_guard<std::mutex> head_lock(head_mutex_);
        if (head_.get() == get_tail()) {  // 锁内重查，避免真的弹出空队列
            return std::nullopt;
        }
        std::unique_ptr<Node> old_head = std::move(head_);
        head_ = std::move(old_head->next);
        return std::move(old_head->data);
    }
    // ANCHOR_END: wrong

private:
    Node* get_tail() {
        std::lock_guard<std::mutex> tail_lock(tail_mutex_);
        return tail_;
    }

    std::mutex head_mutex_;
    std::mutex tail_mutex_;
    std::unique_ptr<Node> head_;
    Node* tail_;
};

}  // namespace

int main() {
    constexpr int kProducers = 2;
    constexpr int kPerProducer = 2000;
    constexpr int kTotal = kProducers * kPerProducer;

    WrongQueue<int> queue;
    std::atomic<bool> producers_done{false};

    std::vector<std::thread> producers;
    for (int p = 0; p < kProducers; ++p) {
        producers.emplace_back([&, p] {
            for (int i = 0; i < kPerProducer; ++i) {
                queue.push(p * kPerProducer + i);
            }
        });
    }

    std::vector<std::thread> consumers;
    for (int c = 0; c < 2; ++c) {
        consumers.emplace_back([&] {
            int n = 0;
            while (!producers_done.load() || n < kTotal / 2) {
                if (queue.wrong_try_pop()) {
                    ++n;
                } else {
                    std::this_thread::yield();
                }
                if (n == kTotal / 2) {
                    break;
                }
            }
        });
    }

    for (auto& thread : producers) {
        thread.join();
    }
    producers_done.store(true);
    for (auto& thread : consumers) {
        thread.join();
    }
    return 0;
}
