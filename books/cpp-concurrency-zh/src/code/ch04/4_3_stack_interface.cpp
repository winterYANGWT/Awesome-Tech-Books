// mode: run
// expect: 同一个 top 值被两个线程拿到：1000 轮
// 每个成员函数都加了锁，但 empty()/top()/pop() 组合起来仍然不是原子的。
// 这里用 barrier 把两个线程摆到固定的交错上，每轮都复现。
#include <barrier>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// ANCHOR: api
template <class T>
class LockedStack {
public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(m_);
        data_.push_back(value);
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(m_);
        return data_.empty();
    }

    T top() {
        std::lock_guard<std::mutex> lock(m_);
        return data_.back();
    }

    bool pop(T& out) {
        std::lock_guard<std::mutex> lock(m_);
        if (data_.empty()) {
            return false;
        }
        out = data_.back();
        data_.pop_back();
        return true;
    }

private:
    std::mutex m_;
    std::vector<T> data_;
};
// ANCHOR_END: api

int main() {
    constexpr int kRounds = 1000;
    LockedStack<int> stack;
    std::barrier sync(3);  // 主线程和两个工作线程
    std::vector<int> tops(kRounds * 2, -1);
    std::vector<int> popped(kRounds * 2, -1);

    auto body = [&](int id) {
        for (int round = 0; round < kRounds; ++round) {
            sync.arrive_and_wait();  // 主线程已放入一个元素
            const bool is_empty = stack.empty();
            sync.arrive_and_wait();  // 两个线程都拿到了 empty() 的结果
            if (!is_empty) {
                tops[round * 2 + id] = stack.top();
            }
            sync.arrive_and_wait();  // 两个线程都 top 过了
            int value = -1;
            if (stack.pop(value)) {
                popped[round * 2 + id] = value;
            }
            sync.arrive_and_wait();  // 本轮结束
        }
    };

    std::thread a(body, 0);
    std::thread b(body, 1);
    for (int round = 0; round < kRounds; ++round) {
        stack.push(round);
        sync.arrive_and_wait();
        sync.arrive_and_wait();
        sync.arrive_and_wait();
        sync.arrive_and_wait();
    }
    a.join();
    b.join();

    int same_top = 0;
    int empty_pop = 0;
    for (int round = 0; round < kRounds; ++round) {
        if (tops[round * 2] == tops[round * 2 + 1]) {
            ++same_top;
        }
        if (popped[round * 2] < 0) {
            ++empty_pop;
        }
        if (popped[round * 2 + 1] < 0) {
            ++empty_pop;
        }
    }
    std::cout << kRounds << " 轮，每轮先放入一个元素，再让两个线程各走一遍 empty/top/pop\n";
    std::cout << "同一个 top 值被两个线程拿到：" << same_top << " 轮\n";
    std::cout << "第二次 pop 取到空栈：" << empty_pop << " 轮\n";
    std::cout << "按 top 之后就算处理来算，同一个值被处理两次：" << same_top << " 次\n";
}
