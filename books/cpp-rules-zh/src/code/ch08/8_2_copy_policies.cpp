// 第 8.2 节：同一个资源管理器，四种复制语义各自负责什么
#include <cstddef>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>

// 用计数把“锁了几次”变成可观察的状态；真实代码直接看 mutex 是否可锁。
class Gate {
public:
    void lock() {
        mutex_.lock();
        ++depth_;
    }
    void unlock() {
        --depth_;
        mutex_.unlock();
    }
    int depth() const { return depth_; }

private:
    std::mutex mutex_;
    int depth_ = 0;
};

// 共享所有权：拷贝 SharedPass 只增加 shared_ptr 的计数，不重复加锁；
// 最后一个 shared_ptr 消失时，删除器执行 unlock 而不是 delete。
class SharedPass {
public:
    explicit SharedPass(std::shared_ptr<Gate> gate) : gate_(std::move(gate)) {
        gate_->lock();
    }
    long owners() const { return gate_.use_count(); }

private:
    std::shared_ptr<Gate> gate_;
};

// 深拷贝：每个 Buffer 拥有自己的数组。
class Buffer {
public:
    explicit Buffer(std::size_t n) : n_(n), data_(std::make_unique<int[]>(n)) {}

    Buffer(const Buffer& other) : n_(other.n_), data_(std::make_unique<int[]>(n_)) {
        for (std::size_t i = 0; i < n_; ++i) {
            data_[i] = other.data_[i];
        }
    }

    Buffer& operator=(const Buffer& other) {
        if (this != &other) {
            Buffer tmp(other);
            std::swap(n_, tmp.n_);
            std::swap(data_, tmp.data_);
        }
        return *this;
    }

    void set(std::size_t i, int value) { data_[i] = value; }
    int get(std::size_t i) const { return data_[i]; }

private:
    std::size_t n_;
    std::unique_ptr<int[]> data_;
};

// 独占所有权：拷贝被删除，只能移动。
class Exclusive {
public:
    explicit Exclusive(std::string name) : name_(std::move(name)) {}
    Exclusive(Exclusive&&) noexcept = default;
    Exclusive& operator=(Exclusive&&) noexcept = default;
    Exclusive(const Exclusive&) = delete;
    Exclusive& operator=(const Exclusive&) = delete;

    const std::string& name() const { return name_; }

private:
    std::string name_;
};

int main() {
    std::cout << "共享所有权：\n";
    Gate gate;
    // 删除器绑定在控制块里：计数归零时调用 unlock，而不是 delete。
    // 先检查 depth，避免 lock 自己抛异常时删除器去 unlock 一把没锁上的锁。
    std::shared_ptr<Gate> shared(&gate, [](Gate* g) {
        if (g->depth() > 0) {
            g->unlock();
        }
    });
    {
        SharedPass p1(shared);
        SharedPass p2 = p1;  // 复制：共享同一个 Gate
        std::cout << "  两个 Pass 的 use_count = " << p1.owners() << "\n";
    }
    std::cout << "  Pass 作用域结束后 depth = " << gate.depth()
              << "（外部 shared_ptr 还持有）\n";
    shared.reset();
    std::cout << "  reset 后 depth = " << gate.depth() << "\n";

    std::cout << "深拷贝：\n";
    Buffer original(3);
    original.set(0, 1);
    Buffer copy = original;
    copy.set(0, 99);
    std::cout << "  original[0] = " << original.get(0)
              << "，copy[0] = " << copy.get(0) << "\n";

    std::cout << "独占所有权：\n";
    static_assert(!std::is_copy_constructible_v<Exclusive>);
    static_assert(std::is_move_constructible_v<Exclusive>);
    Exclusive owner("ledger");
    Exclusive moved = std::move(owner);
    std::cout << "  转移后 name = " << moved.name() << "\n";
}
