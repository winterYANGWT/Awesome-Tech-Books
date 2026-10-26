// 第 8.1 节：标准库已经提供了锁和内存的 RAII 包装
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

struct Widget {
    std::string name;
    explicit Widget(std::string n) : name(std::move(n)) {}
    ~Widget() { std::cout << "  析构 Widget(" << name << ")\n"; }
};

// 离开作用域（包括异常展开）时 lock_guard 解锁。
void update_shared_state(std::mutex& m) {
    std::lock_guard<std::mutex> lock(m);
    std::cout << "  持锁修改共享状态\n";
    throw std::runtime_error("修改失败");
}

// C++17 的 scoped_lock 一次锁住多个 mutex，并按标准算法避免死锁。
void update_two(std::mutex& a, std::mutex& b) {
    std::scoped_lock lock(a, b);
    std::cout << "  同时持有两个锁\n";
}

// 提前 return 也绕不过 unique_ptr 的析构函数。
int use_widget(bool early_return) {
    std::unique_ptr<Widget> w = std::make_unique<Widget>("panel");
    std::cout << "  使用 " << w->name << "\n";
    if (early_return) {
        return 1;
    }
    return 0;
}

int main() {
    std::mutex m;
    try {
        update_shared_state(m);
    } catch (const std::exception& e) {
        std::cout << "捕获：" << e.what() << "\n";
    }
    const bool free_after_throw = m.try_lock();
    if (free_after_throw) {
        m.unlock();
    }
    std::cout << "异常之后 try_lock 成功：" << std::boolalpha
              << free_after_throw << "\n";

    std::mutex a;
    std::mutex b;
    update_two(a, b);  // scoped_lock 在函数返回时已经自动解锁

    std::cout << "提前 return 的路径：\n";
    use_widget(true);
    std::cout << "函数返回后对象已经不在\n";
}
