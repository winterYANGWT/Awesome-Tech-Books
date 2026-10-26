// 第 30.1 节：每个成员函数各自加锁，仍保不住跨调用的复合操作
// flags: -pthread
#include <cstddef>
#include <future>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

// 内部锁：每个公开成员函数都自己加锁。单次调用是安全的，
// 但“先 size() 再 pop()”这种复合操作仍然会被别的线程插进来。
class OrderQueue {
public:
    void push(int id) {
        std::lock_guard<std::mutex> guard(m_);
        items_.push_back(id);
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> guard(m_);
        return items_.size();
    }

    int pop() {
        std::lock_guard<std::mutex> guard(m_);
        if (items_.empty()) throw std::out_of_range("queue is empty");
        const int id = items_.front();
        items_.erase(items_.begin());
        return id;
    }

private:
    mutable std::mutex m_;
    std::vector<int> items_;
};

int main() {
    OrderQueue q;
    q.push(10);                                  // 队列里只有一张订单

    std::promise<void> sized, popped;
    auto sized_future = sized.get_future();
    auto popped_future = popped.get_future();

    std::thread a([&] {
        const std::size_t n = q.size();          // 锁只覆盖这一次 size()
        std::cout << "A: size() 报告 " << n << " 单\n";
        sized.set_value();
        popped_future.wait();                    // 等 B 先取走
        try {
            q.pop();                             // 这一次 pop() 也会加锁，但已经晚了
            std::cout << "A: pop 成功\n";
        } catch (const std::out_of_range& e) {
            std::cout << "A: pop 抛异常: " << e.what() << '\n';
        }
    });

    std::thread b([&] {
        sized_future.wait();
        std::cout << "B: 取走 " << q.pop() << '\n';
        popped.set_value();
    });

    a.join();
    b.join();
    std::cout << "最终剩余 " << q.size() << " 单\n";
}
