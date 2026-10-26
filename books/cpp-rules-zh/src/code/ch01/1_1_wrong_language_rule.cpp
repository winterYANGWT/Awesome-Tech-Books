// 第 1.1 节：把 C 的结构体按值拷贝规则套到管理资源的类上，析构两次
// mode: crash
// expect: double-free

#include <cstddef>
#include <cstdio>

class Buffer {
public:
    explicit Buffer(std::size_t n) : data_(new int[n]{}), size_(n) {}
    ~Buffer() { delete[] data_; }

    // C 的结构体复制就是逐字节搬成员；对持有指针的类，这叫做浅拷贝。
    Buffer(const Buffer&) = default;
    Buffer& operator=(const Buffer&) = default;

    int& at(std::size_t i) { return data_[i]; }
    std::size_t size() const { return size_; }

private:
    int* data_;
    std::size_t size_;
};

// C 的习惯：结构体按值传，复制一份不会出事。到了面向对象子语言里，
// 复制的是指针，两个 Buffer 的析构函数都会 delete 同一块内存。
void fill(Buffer buffer) {
    for (std::size_t i = 0; i < buffer.size(); ++i) {
        buffer.at(i) = static_cast<int>(i + 1);
    }
}

int main() {
    Buffer buffer(4);
    fill(buffer);  // fill 返回时局部 buffer 先析构，main 里的 buffer 已经悬垂
    std::puts("filled");
}
