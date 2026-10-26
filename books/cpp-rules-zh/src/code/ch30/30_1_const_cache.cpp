// 第 30.1 节：const 成员函数里的 mutable 缓存必须自己加锁
// flags: -pthread
#include <algorithm>
#include <array>
#include <barrier>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

// 二次多项式 a*x^2 + b*x + c 的根。roots() 不改变多项式的值，逻辑上是读操作，
// 但缓存要求它写入 mutable 成员，所以“const”并不等于“并发安全”。
class Polynomial {
public:
    using RootsType = std::vector<double>;

    Polynomial(double a, double b, double c) : a_(a), b_(b), c_(c) {}

    RootsType roots() const {
        std::lock_guard<std::mutex> guard(m_);   // 缓存检查和更新是一个整体
        if (!valid_) {
            ++computations_;
            const double disc = b_ * b_ - 4 * a_ * c_;
            const double sq = std::sqrt(disc);
            values_ = {(-b_ + sq) / (2 * a_), (-b_ - sq) / (2 * a_)};
            std::sort(values_.begin(), values_.end());
            valid_ = true;
        }
        return values_;                          // 返回副本，调用方拿不到内部状态
    }

    int computations() const {
        std::lock_guard<std::mutex> guard(m_);
        return computations_;
    }

private:
    double a_, b_, c_;
    mutable std::mutex m_;                       // 锁本身在 const 函数里也要改
    mutable bool valid_ = false;
    mutable RootsType values_;
    mutable int computations_ = 0;
};

// mutex 既不可复制也不可移动，含 mutex 的类连移动构造也不会隐式生成。
static_assert(!std::is_copy_constructible_v<Polynomial>);
static_assert(!std::is_move_constructible_v<Polynomial>);

int main() {
    const Polynomial p(1, -3, 2);                // x^2 - 3x + 2，根为 1 和 2
    constexpr int kThreads = 4;
    std::array<Polynomial::RootsType, kThreads> results;
    std::barrier start(kThreads);

    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i] {
            start.arrive_and_wait();             // 尽量让四个线程同时进入 roots()
            results[i] = p.roots();              // 每个线程写自己的数组元素
        });
    }
    for (auto& t : threads) t.join();

    bool same = true;
    for (int i = 1; i < kThreads; ++i) same = same && (results[i] == results[0]);

    std::cout << "roots: " << results[0][0] << ' ' << results[0][1] << '\n';
    std::cout << "const 调用次数: " << p.computations() << '\n';
    std::cout << kThreads << " 个线程结果一致: " << std::boolalpha << same << '\n';
    std::cout << "可复制: " << std::is_copy_constructible_v<Polynomial> << '\n';
}
