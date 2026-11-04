// mode: run
// expect: 违反层级
#include <climits>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

// 层级锁不是标准类型，只用来演示“运行时检查加锁顺序”。
class hierarchical_mutex {
public:
    explicit hierarchical_mutex(unsigned long value) : level_(value) {}

    void lock() {
        check();
        internal_.lock();
        previous_ = current_level_;
        current_level_ = level_;
    }

    bool try_lock() {
        check();
        if (!internal_.try_lock()) return false;
        previous_ = current_level_;
        current_level_ = level_;
        return true;
    }

    void unlock() {
        internal_.unlock();
        current_level_ = previous_;
    }

private:
    void check() const {
        if (level_ >= current_level_) {
            throw std::logic_error("违反层级：请求 " + std::to_string(level_) +
                                   "，当前 " + std::to_string(current_level_));
        }
    }

    std::mutex internal_;
    unsigned long level_;
    unsigned long previous_ = 0;
    static thread_local unsigned long current_level_;
};

thread_local unsigned long hierarchical_mutex::current_level_ = ULONG_MAX;

int main() {
    hierarchical_mutex high(10000), mid(5000), other(6000);

    std::thread a([&] {
        high.lock();
        std::cout << "线程 A：锁 10000\n";
        mid.lock();
        std::cout << "线程 A：锁 5000（10000 -> 5000，允许）\n";
        mid.unlock();
        high.unlock();
    });
    a.join();

    std::thread b([&] {
        other.lock();
        std::cout << "线程 B：锁 6000\n";
        try {
            high.lock();
            std::cout << "线程 B：锁 10000（不应出现）\n";
            high.unlock();
        } catch (const std::logic_error& e) {
            std::cout << "线程 B 违规：" << e.what() << "\n";
        }
        other.unlock();
    });
    b.join();
}
