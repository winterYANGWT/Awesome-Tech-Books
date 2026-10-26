// 第 6.4 节：写了析构就会抑制移动；五件套要成套考虑
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

struct Counted {
    static inline int copies = 0;
    static inline int moves = 0;

    Counted() = default;
    Counted(const Counted&) { ++copies; }
    Counted(Counted&&) noexcept { ++moves; }
    Counted& operator=(const Counted&) { ++copies; return *this; }
    Counted& operator=(Counted&&) noexcept { ++moves; return *this; }
};

void reset() { Counted::copies = 0; Counted::moves = 0; }

// 持有资源的类：五件套全部显式写出，移动标 noexcept。
class Buffer {
public:
    explicit Buffer(int size) : data_(static_cast<std::size_t>(size), 0) {}

    ~Buffer() = default;   // 用户声明，但明确要默认版本

    Buffer(const Buffer& rhs) : data_(rhs.data_) { ++Counted::copies; }
    Buffer(Buffer&& rhs) noexcept : data_(std::move(rhs.data_)) { ++Counted::moves; }

    Buffer& operator=(const Buffer& rhs) {
        data_ = rhs.data_;
        ++Counted::copies;
        return *this;
    }
    Buffer& operator=(Buffer&& rhs) noexcept {
        data_ = std::move(rhs.data_);
        ++Counted::moves;
        return *this;
    }

private:
    std::vector<int> data_;
};

// 只加了一个析构函数：移动操作不再生成，vector 扩容时改回拷贝。
class LegacyBuffer {
public:
    explicit LegacyBuffer(int size) : data_(static_cast<std::size_t>(size), 0) {}
    ~LegacyBuffer() {}   // 本来只是想打日志

    std::vector<int> data_;
    Counted marker;
};

// 只允许移动：拷贝操作被删除，移动操作显式默认。
class MoveOnly {
public:
    MoveOnly() = default;
    MoveOnly(MoveOnly&&) = default;
    MoveOnly& operator=(MoveOnly&&) = default;
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;

private:
    std::unique_ptr<int> value_ = std::make_unique<int>(0);
};

// 零法则：不写任何特殊成员，资源交给成员自己管理。
class RuleOfZero {
public:
    RuleOfZero() = default;

private:
    std::string name_;
    std::vector<int> values_;
};

static_assert(!std::is_copy_constructible_v<MoveOnly>);
static_assert(std::is_nothrow_move_constructible_v<MoveOnly>);
static_assert(std::is_copy_constructible_v<RuleOfZero>);
static_assert(std::is_nothrow_move_constructible_v<RuleOfZero>);

int main() {
    reset();
    {
        std::vector<Buffer> buffers;
        buffers.reserve(1);
        for (int i = 0; i < 4; ++i) {
            buffers.push_back(Buffer(i));
        }
    }
    std::cout << "Buffer（五件套）: copies=" << Counted::copies
              << " moves=" << Counted::moves << '\n';

    reset();
    {
        std::vector<LegacyBuffer> legacy;
        legacy.reserve(1);
        for (int i = 0; i < 4; ++i) {
            legacy.push_back(LegacyBuffer(i));
        }
    }
    std::cout << "LegacyBuffer（只有析构）: copies=" << Counted::copies
              << " moves=" << Counted::moves << '\n';

    std::cout << "MoveOnly: copyable=" << std::is_copy_constructible_v<MoveOnly>
              << " movable=" << std::is_nothrow_move_constructible_v<MoveOnly> << '\n';
    std::cout << "RuleOfZero: copyable=" << std::is_copy_constructible_v<RuleOfZero>
              << " movable=" << std::is_nothrow_move_constructible_v<RuleOfZero> << '\n';
}
