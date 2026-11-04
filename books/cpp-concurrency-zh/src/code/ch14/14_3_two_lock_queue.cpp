// mode: run
// expect: 入队 40000，出队 40000，重复 0，缺失 0
// 头尾各一把锁，中间用哑节点隔开；4 个生产者、4 个消费者。
#include <atomic>
#include <cstdio>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

namespace {

template <class T>
class TwoLockQueue {
    struct Node {
        std::optional<T> data;
        std::unique_ptr<Node> next;
    };

public:
    TwoLockQueue() : head_(std::make_unique<Node>()), tail_(head_.get()) {}

    void push(T value) {
        auto new_node = std::make_unique<Node>();
        Node* const new_tail = new_node.get();
        {
            std::lock_guard<std::mutex> tail_lock(tail_mutex_);
            tail_->data = std::move(value);  // 数据写进旧哑节点
            tail_->next = std::move(new_node);  // 新节点留作哑尾
            tail_ = new_tail;
        }
    }

    std::optional<T> try_pop() {
        std::unique_ptr<Node> old_head = pop_head();
        if (!old_head) {
            return std::nullopt;
        }
        return std::move(old_head->data);
    }

    // 引用输出：先赋值、再摘节点。赋值抛异常时 head_ 还没动，节点仍在队列里；
    // 但移动赋值可能已经改过源值，值不保证还是原值。
    bool pop(T& out) {
        std::lock_guard<std::mutex> head_lock(head_mutex_);
        if (head_.get() == get_tail()) {
            return false;
        }
        out = std::move(head_->next->data.value());
        std::unique_ptr<Node> old_head = std::move(head_);
        head_ = std::move(old_head->next);
        return true;
    }

private:
    Node* get_tail() {
        std::lock_guard<std::mutex> tail_lock(tail_mutex_);
        return tail_;
    }

    std::unique_ptr<Node> pop_head() {
        std::lock_guard<std::mutex> head_lock(head_mutex_);
        if (head_.get() == get_tail()) {
            return nullptr;  // 只有哑节点，队列为空
        }
        std::unique_ptr<Node> old_head = std::move(head_);
        head_ = std::move(old_head->next);
        return old_head;
    }

    std::mutex head_mutex_;
    std::mutex tail_mutex_;
    std::unique_ptr<Node> head_;
    Node* tail_;
};

}  // namespace

int main() {
    constexpr int kProducers = 4;
    constexpr int kConsumers = 4;
    constexpr int kPerProducer = 10000;
    constexpr int kTotal = kProducers * kPerProducer;

    TwoLockQueue<int> queue;
    std::vector<std::atomic<int>> seen(kTotal);
    for (auto& count : seen) {
        count.store(0, std::memory_order_relaxed);
    }
    std::atomic<int> popped{0};

    std::vector<std::thread> producers;
    for (int p = 0; p < kProducers; ++p) {
        producers.emplace_back([&, p] {
            for (int i = 0; i < kPerProducer; ++i) {
                queue.push(p * kPerProducer + i);
            }
        });
    }

    std::vector<std::thread> consumers;
    for (int c = 0; c < kConsumers; ++c) {
        consumers.emplace_back([&] {
            std::vector<int> local;
            local.reserve(kTotal / kConsumers);
            while (static_cast<int>(local.size()) < kTotal / kConsumers) {
                if (auto value = queue.try_pop()) {
                    local.push_back(*value);
                } else {
                    std::this_thread::yield();  // 队列暂时空，让生产者跑
                }
            }
            for (int value : local) {
                seen[value].fetch_add(1, std::memory_order_relaxed);
            }
            popped.fetch_add(static_cast<int>(local.size()), std::memory_order_relaxed);
        });
    }

    for (auto& thread : producers) {
        thread.join();
    }
    for (auto& thread : consumers) {
        thread.join();
    }

    int duplicates = 0;
    int missing = 0;
    for (int value = 0; value < kTotal; ++value) {
        const int count = seen[value].load(std::memory_order_relaxed);
        if (count == 0) {
            ++missing;
        } else if (count > 1) {
            ++duplicates;
        }
    }

    std::printf("入队 %d，出队 %d，重复 %d，缺失 %d\n", kTotal, popped.load(), duplicates,
                missing);
    return 0;
}
