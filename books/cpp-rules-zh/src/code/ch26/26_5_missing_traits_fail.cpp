// 第 26.5 节：只写 !=/*/++ 的迭代器进不了标准算法，iterator_traits 缺类型
// mode: fail
// expect: difference_type
#include <iterator>

class Countdown {
public:
    Countdown() = default;
    explicit Countdown(int n) : n_(n) {}
    int operator*() const { return n_; }
    Countdown& operator++() {
        --n_;
        return *this;
    }
    bool operator!=(const Countdown& other) const { return n_ != other.n_; }

private:
    int n_ = 0;
};

int main() {
    Countdown first(3), last(0);
    return static_cast<int>(std::distance(first, last));
}
