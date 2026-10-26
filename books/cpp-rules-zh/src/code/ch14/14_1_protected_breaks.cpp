// 第 14.1 节：protected 数据把不变式的维护权交给无界的派生类
// mode: crash
// expect: used=6
#include <cstddef>
#include <iostream>

class Buffer {
public:
    bool push(int v) {
        if (used_ == kCapacity) return false;
        data_[used_++] = v;
        return true;
    }
    [[nodiscard]] std::size_t used() const { return used_; }

protected:
    // 不变式：used_ <= kCapacity
    static constexpr std::size_t kCapacity = 4;
    int data_[kCapacity]{};
    std::size_t used_ = 0;
};

class Inflater : public Buffer {
public:
    // 派生类看得见 used_，不需要经过 push，本来可以忽略的不变式被直接改写。
    void inflate() { used_ = kCapacity + 2; }
};

int main() {
    Inflater b;
    b.push(1);
    b.inflate();
    std::cout << "used=" << b.used() << " capacity=4\n";
    b.push(2);   // used_ 是 6，push 只挡 == 4，于是写 data_[6]
}
