// 第 23.4 节：把谓词的 operator() 写成 const，至少让编译器挡住改成员
// mode: fail
// expect: const
#include <cstddef>

class FlagNth {
public:
    explicit FlagNth(std::size_t n) : n_(n) {}

    bool operator()(int) const { return ++calls_ == n_; }

private:
    std::size_t calls_ = 0;
    std::size_t n_;
};

int main() {
    FlagNth third(3);
    return third(1) ? 0 : 1;
}
