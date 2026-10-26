// 第 28.1 节：比较精确增长、固定增量、几何增长与一次预留的分配和复制次数
#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <iostream>

class GrowingText {
public:
    enum class Policy { exact, fixed, geometric, reserved };
    using Stats = std::pair<std::size_t, std::size_t>;  // 分配次数，字符复制次数

    explicit GrowingText(Policy policy) : policy_(policy) {}
    ~GrowingText() { delete[] buf_; }

    GrowingText(const GrowingText&) = delete;
    GrowingText& operator=(const GrowingText&) = delete;

    void append(char c) {
        if (used_ == cap_) {
            grow(used_ + 1);
        }
        buf_[used_++] = c;
    }

    std::size_t size() const { return used_; }
    std::size_t capacity() const { return cap_; }
    Stats stats() const { return {allocations_, copies_}; }

private:
    void grow(std::size_t needed) {
        std::size_t newCapacity = 0;
        switch (policy_) {
        case Policy::exact:
            newCapacity = needed;
            break;
        case Policy::fixed:
            newCapacity = cap_ + 32;
            break;
        case Policy::geometric:
            // 约 1.5 倍，并保证至少够这次使用；从 0 开始时至少长到 1。
            newCapacity = std::max({cap_ * 3 / 2, cap_ + 1, needed});
            break;
        case Policy::reserved:
            newCapacity = 1200;
            break;
        }

        char* next = new char[newCapacity];
        for (std::size_t i = 0; i < used_; ++i) {
            next[i] = buf_[i];
        }
        copies_ += used_;

        // 先完成分配和复制，再释放旧缓冲区；中途抛异常时对象仍然完整。
        delete[] buf_;
        buf_ = next;
        cap_ = newCapacity;
        ++allocations_;
    }

    Policy policy_;
    char* buf_ = nullptr;
    std::size_t cap_ = 0;
    std::size_t used_ = 0;
    std::size_t allocations_ = 0;
    std::size_t copies_ = 0;
};

void report(const char* name, GrowingText::Policy policy) {
    GrowingText text(policy);
    for (int i = 0; i < 1200; ++i) {
        text.append('a');
    }
    const auto [allocations, copies] = text.stats();
    std::cout << std::left << std::setw(10) << name
              << " size=" << text.size()
              << " capacity=" << text.capacity()
              << " 分配=" << allocations
              << " 复制字符=" << copies
              << " 每字符平均复制=" << std::fixed << std::setprecision(1)
              << static_cast<double>(copies) / static_cast<double>(text.size()) << '\n';
}

int main() {
    report("精确增长", GrowingText::Policy::exact);
    report("固定增量", GrowingText::Policy::fixed);
    report("几何增长", GrowingText::Policy::geometric);
    report("一次预留", GrowingText::Policy::reserved);
}
