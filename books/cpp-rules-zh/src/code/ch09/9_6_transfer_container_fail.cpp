// 第 9.6 节：拷贝即转移的类型不满足容器“拷贝后源不变”的契约
// mode: fail
// expect: push_back
#include <vector>

template <typename T>
class TransferPtr {
public:
    explicit TransferPtr(T* p = nullptr) : p_(p) {}
    ~TransferPtr() { delete p_; }
    TransferPtr(TransferPtr& other) : p_(other.p_) { other.p_ = nullptr; }
    TransferPtr& operator=(TransferPtr& other) {
        if (this != &other) {
            delete p_;
            p_ = other.p_;
            other.p_ = nullptr;
        }
        return *this;
    }

private:
    T* p_;
};

int main() {
    std::vector<TransferPtr<int>> values;
    values.push_back(TransferPtr<int>(new int(1)));
    values.push_back(TransferPtr<int>(new int(2)));
}
