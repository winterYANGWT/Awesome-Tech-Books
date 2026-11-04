// mode: crash
// expect: pop\(\) on empty stack
// 每个成员函数都加了锁，但 empty() 和 pop() 之间状态会被另一个线程改掉。
#include <barrier>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <vector>

class Stack {
public:
    void push(int value) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_.push_back(value);
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.empty();
    }

    int pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (data_.empty()) {
            // 真实实现里这里会读越界，是未定义行为；为了能稳定复现，改成显式崩溃。
            std::fputs("pop() on empty stack\n", stderr);
            std::abort();
        }
        const int value = data_.back();
        data_.pop_back();
        return value;
    }

private:
    std::mutex mutex_;
    std::vector<int> data_;
};

int main() {
    Stack stack;
    stack.push(7);
    std::barrier gate{2};

    auto worker = [&stack, &gate] {
        const bool not_empty = !stack.empty();  // 检查和取数之间没有锁
        gate.arrive_and_wait();                 // 让两个线程都检查完再动手
        if (not_empty) {
            (void)stack.pop();
        }
    };

    std::thread a(worker);
    std::thread b(worker);
    a.join();
    b.join();
}
