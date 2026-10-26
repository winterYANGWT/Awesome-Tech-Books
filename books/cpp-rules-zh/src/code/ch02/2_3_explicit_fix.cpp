// 第 2.3 节：反例：构造函数加 explicit 后，漏写下标的比较不再编译
// mode: fail
// expect: invalid operands
#include <cstddef>
#include <vector>

class Buffer {
public:
    explicit Buffer(std::size_t n) : data_(n, 0) {}

    int operator[](std::size_t i) const { return data_[i]; }
    bool operator==(const Buffer& other) const { return data_ == other.data_; }

private:
    std::vector<int> data_;
};

int main() {
    Buffer a(4), b(4);
    bool same = (a == b[0]);   // b[0] 是 int，不再隐式变成 Buffer
    return same ? 0 : 1;
}
