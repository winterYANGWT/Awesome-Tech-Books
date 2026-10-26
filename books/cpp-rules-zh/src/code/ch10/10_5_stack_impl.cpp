// 第 10.5 节：分配与构造分离，容器缓冲区只构造真正用到的元素
#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

// 故意不提供默认构造函数，模拟“只在需要时构造”的类型
class Record {
public:
    explicit Record(int value) : value_(value) {
        ++alive_;
        std::cout << "  构造 Record(" << value_ << ")\n";
    }
    ~Record() {
        --alive_;
        std::cout << "  析构 Record(" << value_ << ")\n";
    }
    Record(const Record&) = delete;
    Record& operator=(const Record&) = delete;

    int value() const { return value_; }
    static int alive() { return alive_; }

private:
    int value_;
    inline static int alive_ = 0;
};

template <class T>
class Buffer {
public:
    explicit Buffer(std::size_t capacity) : capacity_(capacity) {
        if (capacity_ != 0) data_ = Traits::allocate(alloc_, capacity_);
    }
    ~Buffer() {
        clear();
        if (data_ != nullptr) Traits::deallocate(alloc_, data_, capacity_);
    }
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    template <class... Args>
    void emplace_back(Args&&... args) {
        if (size_ == capacity_) throw std::length_error("Buffer 已满");
        // 在未构造的槽位上构造，容量里没有用到的槽位保持未构造
        std::construct_at(data_ + size_, std::forward<Args>(args)...);
        ++size_;
    }

    void pop_back() {
        std::destroy_at(data_ + (size_ - 1));
        --size_;
    }

    void clear() {
        while (size_ != 0) pop_back();
    }

    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }

private:
    using Traits = std::allocator_traits<std::allocator<T>>;

    std::allocator<T> alloc_;
    T* data_ = nullptr;
    std::size_t capacity_ = 0;
    std::size_t size_ = 0;
};

int main() {
    std::cout << "1) 分配 4 个槽位，没有构造任何对象\n";
    Buffer<Record> buffer(4);
    std::cout << "  容量 " << buffer.capacity() << "，已构造 " << Record::alive() << "\n";

    std::cout << "2) 按需构造两个\n";
    buffer.emplace_back(10);
    buffer.emplace_back(20);
    std::cout << "  size=" << buffer.size() << "，已构造 " << Record::alive() << "\n";

    std::cout << "3) pop_back 只析构最后一个\n";
    buffer.pop_back();
    std::cout << "  size=" << buffer.size() << "，已构造 " << Record::alive() << "\n";

    std::cout << "4) Buffer 析构：先析构剩余元素，再归还整块存储\n";
}
