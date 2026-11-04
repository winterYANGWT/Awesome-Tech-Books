// mode: run
// expect: 同一个值被取到两次：0 次
// 把 empty()/top()/pop() 合并成 try_pop()，判断和取出在同一把锁里完成。
#include <barrier>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// ANCHOR: api
template <class T>
class ThreadSafeStack {
public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(m_);
        data_.push_back(std::move(value));
        cv_.notify_one();
    }

    bool try_pop(T& out) {
        std::lock_guard<std::mutex> lock(m_);
        if (data_.empty()) {
            return false;
        }
        out = std::move(data_.back());
        data_.pop_back();
        return true;
    }

    T wait_pop() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [this] { return !data_.empty(); });
        T out = std::move(data_.back());
        data_.pop_back();
        return out;
    }

private:
    std::mutex m_;
    std::condition_variable cv_;
    std::vector<T> data_;
};
// ANCHOR_END: api

int main() {
    constexpr int kRounds = 1000;
    ThreadSafeStack<int> stack;
    std::barrier sync(3);
    std::vector<int> got(kRounds * 2, -1);

    auto body = [&](int id) {
        for (int round = 0; round < kRounds; ++round) {
            sync.arrive_and_wait();  // 主线程已放入一个元素
            int value = -1;
            if (stack.try_pop(value)) {
                got[round * 2 + id] = value;
            }
            sync.arrive_and_wait();  // 两个线程都尝试过了
        }
    };

    std::thread a(body, 0);
    std::thread b(body, 1);
    for (int round = 0; round < kRounds; ++round) {
        stack.push(round);
        sync.arrive_and_wait();
        sync.arrive_and_wait();
    }
    a.join();
    b.join();

    int successes = 0;
    int duplicates = 0;
    for (int round = 0; round < kRounds; ++round) {
        const int first = got[round * 2];
        const int second = got[round * 2 + 1];
        successes += (first >= 0) + (second >= 0);
        if (first >= 0 && first == second) {
            ++duplicates;
        }
    }
    std::cout << kRounds << " 轮各放入一个元素，两个线程同时 try_pop\n";
    std::cout << "成功取到：" << successes << " 次，失败：" << (kRounds * 2 - successes) << " 次\n";
    std::cout << "同一个值被取到两次：" << duplicates << " 次\n";

    std::thread waiter([&] {
        const int value = stack.wait_pop();
        std::cout << "wait_pop 在空栈上等到 push 后返回 " << value << '\n';
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    stack.push(42);
    waiter.join();
}
