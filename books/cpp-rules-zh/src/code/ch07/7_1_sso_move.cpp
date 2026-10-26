// 第 7.1 节：短字符串把字符放在对象内部，移动并不比拷贝便宜
#include <array>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string_view>
#include <utility>

// 模拟小字符串优化：字符存在对象内部的缓冲区里，没有可以偷的堆指针。
class InlineText {
public:
    static int bytes_copied;

    explicit InlineText(std::string_view text) : len_(text.size()) {
        for (std::size_t i = 0; i < len_; ++i) {
            buf_[i] = text[i];
        }
    }

    InlineText(const InlineText& other) : buf_(other.buf_), len_(other.len_) {
        bytes_copied += static_cast<int>(len_);
    }

    InlineText(InlineText&& other) noexcept : buf_(other.buf_), len_(other.len_) {
        // 缓冲区在对象内部，偷不走，只能照样复制一遍。
        bytes_copied += static_cast<int>(len_);
        other.len_ = 0;
    }

    std::size_t size() const { return len_; }

private:
    std::array<char, 15> buf_{};
    std::size_t len_;
};
int InlineText::bytes_copied = 0;

// 长字符串把字符放在堆上，移动只搬走指针。
class HeapText {
public:
    static int bytes_copied;
    static int allocations;

    explicit HeapText(std::string_view text)
        : len_(text.size()), data_(std::make_unique<char[]>(len_)) {
        ++allocations;
        for (std::size_t i = 0; i < len_; ++i) {
            data_[i] = text[i];
        }
    }

    HeapText(const HeapText& other)
        : len_(other.len_), data_(std::make_unique<char[]>(other.len_)) {
        ++allocations;
        for (std::size_t i = 0; i < len_; ++i) {
            data_[i] = other.data_[i];
        }
        bytes_copied += static_cast<int>(len_);
    }

    HeapText(HeapText&& other) noexcept
        : len_(other.len_), data_(std::move(other.data_)) {
        other.len_ = 0;
        // 没有复制字符，也没有新的分配。
    }

    std::size_t size() const { return len_; }

private:
    std::size_t len_;
    std::unique_ptr<char[]> data_;
};
int HeapText::bytes_copied = 0;
int HeapText::allocations = 0;

int main() {
    InlineText short_text("order-2024-07");
    InlineText::bytes_copied = 0;
    InlineText short_copy = short_text;
    std::cout << "短文本拷贝：复制 " << InlineText::bytes_copied << " 字节\n";

    InlineText::bytes_copied = 0;
    InlineText short_moved = std::move(short_text);
    std::cout << "短文本移动：复制 " << InlineText::bytes_copied << " 字节\n";

    HeapText long_text("warehouse-A/aisle-12/shelf-07/box-0042");
    HeapText::bytes_copied = 0;
    HeapText::allocations = 0;
    HeapText long_copy = long_text;
    std::cout << "长文本拷贝：复制 " << HeapText::bytes_copied
              << " 字节，分配 " << HeapText::allocations << " 次\n";

    HeapText::bytes_copied = 0;
    HeapText::allocations = 0;
    HeapText long_moved = std::move(long_text);
    std::cout << "长文本移动：复制 " << HeapText::bytes_copied
              << " 字节，分配 " << HeapText::allocations << " 次\n";

    std::cout << "长度：" << short_copy.size() << " / " << short_moved.size()
              << " / " << long_copy.size() << " / " << long_moved.size() << "\n";
}
