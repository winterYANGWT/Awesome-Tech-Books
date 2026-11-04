// mode: run
// expect: 剩余 4571，错误 0
// 每个节点一把 mutex；遍历时先锁下一个节点，再释放当前节点（手递手）。
#include <barrier>
#include <cstdio>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace {

template <class T>
class LockedList {
    struct Node {
        explicit Node(T value) : data(std::move(value)) {}
        T data;
        std::mutex mutex;
        std::unique_ptr<Node> next;
    };

public:
    LockedList() : head_(T{}) {}

    void push_front(T value) {
        auto node = std::make_unique<Node>(std::move(value));
        std::lock_guard<std::mutex> lock(head_.mutex);  // 只锁头节点
        node->next = std::move(head_.next);
        head_.next = std::move(node);
    }

    // ANCHOR: find
    bool find(const T& value) {
        std::unique_lock<std::mutex> lock(head_.mutex);
        Node* current = head_.next.get();
        if (current) {
            std::unique_lock<std::mutex> next_lock(current->mutex);
            lock.unlock();
            lock = std::move(next_lock);
        }
        while (current) {
            if (current->data == value) {
                return true;
            }
            Node* next = current->next.get();  // 在 current 的锁内读 next
            std::unique_lock<std::mutex> next_lock;
            if (next) {
                next_lock = std::unique_lock<std::mutex>(next->mutex);
            }
            lock.unlock();
            lock = std::move(next_lock);
            current = next;
        }
        return false;
    }
    // ANCHOR_END: find

    template <class Function>
    void for_each(Function f) {
        std::unique_lock<std::mutex> lock(head_.mutex);
        Node* current = head_.next.get();
        if (current) {
            std::unique_lock<std::mutex> next_lock(current->mutex);
            lock.unlock();
            lock = std::move(next_lock);
        }
        while (current) {
            f(static_cast<const T&>(current->data));
            Node* next = current->next.get();
            std::unique_lock<std::mutex> next_lock;
            if (next) {
                next_lock = std::unique_lock<std::mutex>(next->mutex);
            }
            lock.unlock();
            lock = std::move(next_lock);
            current = next;
        }
    }

    // ANCHOR: remove
    template <class Predicate>
    void remove_if(Predicate pred) {
        std::unique_lock<std::mutex> prev_lock(head_.mutex);
        Node* prev = &head_;
        Node* current = prev->next.get();
        std::unique_lock<std::mutex> current_lock;
        if (current) {
            current_lock = std::unique_lock<std::mutex>(current->mutex);
        }

        while (current) {
            Node* next = current->next.get();
            std::unique_lock<std::mutex> next_lock;
            if (next) {
                next_lock = std::unique_lock<std::mutex>(next->mutex);
            }

            if (pred(static_cast<const T&>(current->data))) {
                std::unique_ptr<Node> removed = std::move(prev->next);
                prev->next = std::move(current->next);
                current_lock.unlock();
                removed.reset();  // 先解锁，再销毁节点里的 mutex
                current = next;
                current_lock = std::move(next_lock);
            } else {
                prev_lock.unlock();
                prev = current;
                prev_lock = std::move(current_lock);
                current = next;
                current_lock = std::move(next_lock);
            }
        }
    }
    // ANCHOR_END: remove

private:
    Node head_;  // 哑节点，它的锁保护 head_.next
};

constexpr int kValues = 10000;

bool removable(int value) {
    return value % 3 == 0 || value % 5 == 0 || value % 7 == 0;
}

}  // namespace

int main() {
    LockedList<int> list;
    std::barrier pushed{4};

    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < kValues / 4; ++i) {
                list.push_front(t * (kValues / 4) + i);
            }
            pushed.arrive_and_wait();
            if (t == 0) {
                for (int i = 0; i < 5000; ++i) {
                    (void)list.find(i % kValues);
                }
            } else if (t == 1) {
                list.remove_if([](int value) { return value % 3 == 0; });
            } else if (t == 2) {
                list.remove_if([](int value) { return value % 5 == 0; });
            } else {
                list.remove_if([](int value) { return value % 7 == 0; });
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }

    int remaining = 0;
    int wrong = 0;
    list.for_each([&](int value) {
        ++remaining;
        if (removable(value)) {
            ++wrong;
        }
    });

    int expected = 0;
    for (int value = 0; value < kValues; ++value) {
        if (!removable(value)) {
            ++expected;
        }
    }
    if (remaining != expected) {
        ++wrong;
    }

    std::printf("剩余 %d，错误 %d\n", remaining, wrong);
    return 0;
}
