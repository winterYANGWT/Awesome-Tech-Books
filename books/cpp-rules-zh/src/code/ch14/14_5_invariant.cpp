// 第 14.5 节：把不变式写成具体条件，并在每个公开成员返回前检查
#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <utility>

class IntVec {
public:
    IntVec() = default;
    IntVec(const IntVec&) = delete;
    IntVec& operator=(const IntVec&) = delete;
    ~IntVec() { release(); }

    explicit IntVec(std::size_t n, int value) {
        if (n == 0) return;
        data_ = alloc_.allocate(n);
        avail_ = data_;
        limit_ = data_ + n;
        for (; avail_ != limit_; ++avail_) std::construct_at(avail_, value);
        // 这里只存 int，构造不会抛；换成会抛的类型时，要先销毁已构造的部分。
        assert(invariant());
    }

    void push_back(int v) {
        if (avail_ == limit_) grow(capacity() == 0 ? 4 : capacity() * 2);
        std::construct_at(avail_, v);
        ++avail_;
        assert(invariant());
    }

    void pop_back() {
        assert(!empty());
        --avail_;
        std::destroy_at(avail_);
        assert(invariant());
    }

    [[nodiscard]] std::size_t size() const {
        return data_ == nullptr ? 0 : static_cast<std::size_t>(avail_ - data_);
    }
    [[nodiscard]] std::size_t capacity() const {
        return data_ == nullptr ? 0 : static_cast<std::size_t>(limit_ - data_);
    }
    [[nodiscard]] bool empty() const { return size() == 0; }
    [[nodiscard]] bool check_invariant() const { return invariant(); }

    int& operator[](std::size_t i) {
        assert(i < size());
        return data_[i];
    }
    const int& operator[](std::size_t i) const {
        assert(i < size());
        return data_[i];
    }

private:
    // 不变式：
    //   data_ <= avail_ <= limit_；
    //   [data_, avail_) 已构造，[avail_, limit_) 未构造；
    //   三者要么全为空，要么指向同一块分配。
    [[nodiscard]] bool invariant() const {
        if (data_ == nullptr) return avail_ == nullptr && limit_ == nullptr;
        return data_ <= avail_ && avail_ <= limit_;
    }

    void grow(std::size_t new_capacity) {
        int* p = alloc_.allocate(new_capacity);
        int* q = p;
        for (int* it = data_; it != avail_; ++it, ++q) {
            std::construct_at(q, std::move(*it));
        }
        for (int* it = data_; it != avail_; ++it) std::destroy_at(it);
        if (data_ != nullptr) alloc_.deallocate(data_, capacity());
        data_ = p;
        avail_ = q;
        limit_ = p + new_capacity;
        assert(invariant());
    }

    void release() {
        for (int* it = data_; it != avail_; ++it) std::destroy_at(it);
        if (data_ != nullptr) alloc_.deallocate(data_, capacity());
        data_ = avail_ = limit_ = nullptr;
    }

    std::allocator<int> alloc_{};
    int* data_ = nullptr;
    int* avail_ = nullptr;
    int* limit_ = nullptr;
};

int main() {
    IntVec v;
    for (int x : {3, 1, 4, 1, 5, 9}) v.push_back(x);
    v.pop_back();

    std::cout << "size=" << v.size() << " cap=" << v.capacity()
              << " invariant=" << v.check_invariant() << "\n";
    for (std::size_t i = 0; i < v.size(); ++i) {
        std::cout << v[i] << (i + 1 == v.size() ? '\n' : ' ');
    }

    IntVec four(4, 7);
    std::cout << "four: size=" << four.size() << " cap=" << four.capacity()
              << " invariant=" << four.check_invariant() << " values=";
    for (std::size_t i = 0; i < four.size(); ++i) std::cout << four[i] << ' ';
    std::cout << "\n";
}
