// 第 29.5 节：COW 交出引用后必须标记 unshareable，否则旧引用会改到副本
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

// Safe=false 模拟“只加引用计数、不记 unshareable”的天真 COW；
// Safe=true 是补上标记的版本。
template <bool Safe>
class CowText {
public:
    explicit CowText(std::string text)
        : buf_(std::make_shared<Buffer>(std::move(text))) {}

    CowText(const CowText& other) {
        if constexpr (Safe) {
            // 对方把引用交出去过，就不能再共享，直接深拷贝。
            if (!other.buf_->shareable) {
                buf_ = std::make_shared<Buffer>(*other.buf_);
                return;
            }
        }
        buf_ = other.buf_;      // 正常 COW 拷贝：只加引用计数
    }

    CowText& operator=(const CowText& other) {
        if (this != &other) {
            CowText copy(other);
            buf_.swap(copy.buf_);
        }
        return *this;
    }

    ~CowText() = default;

    // 非 const 访问可能被用来写，先保证这块表示只属于自己。
    char& operator[](std::size_t index) {
        detach(true);
        return buf_->text[index];
    }

    const char& operator[](std::size_t index) const {
        return buf_->text[index];
    }

    // 修改操作本身会让旧引用失效，之后可以恢复共享。
    void append(char c) {
        detach(false);
        buf_->text.push_back(c);
    }

    const std::string& str() const { return buf_->text; }
    static int allocations() { return Buffer::allocations; }
    static void reset_allocations() { Buffer::allocations = 0; }

private:
    struct Buffer {
        explicit Buffer(std::string text) : text(std::move(text)) { ++allocations; }
        Buffer(const Buffer& other) : text(other.text) { ++allocations; }

        std::string text;
        bool shareable = true;
        static inline int allocations = 0;
    };

    void detach(bool mark_unshareable) {
        if (buf_.use_count() > 1 || (Safe && !buf_->shareable)) {
            buf_ = std::make_shared<Buffer>(*buf_);   // 克隆一份私有表示
        }
        if constexpr (Safe) {
            buf_->shareable = !mark_unshareable;
        } else {
            (void)mark_unshareable;
        }
    }

    std::shared_ptr<Buffer> buf_;
};

int main() {
    std::cout << "--- 天真 COW：没有 unshareable 标记 ---\n";
    CowText<false>::reset_allocations();
    {
        CowText<false> x("world");
        char& ref = x[1];          // x 此刻独占，引用指向 x 的缓冲区
        CowText<false> y = x;      // 拷贝只加计数，x 和 y 共享
        ref = 'O';                 // 通过旧引用写：y 也跟着变了
        std::cout << "x=" << x.str() << " y=" << y.str()
                  << " allocations=" << CowText<false>::allocations() << '\n';
    }

    std::cout << "--- 补上 unshareable 标记 ---\n";
    CowText<true>::reset_allocations();
    {
        CowText<true> a("hello");
        char& ref = a[1];          // 非 const 访问后，a 标记为不可共享
        CowText<true> b = a;       // 看到标记，深拷贝
        ref = 'E';                 // 只改 a，b 不受影响
        std::cout << "a=" << a.str() << " b=" << b.str()
                  << " allocations=" << CowText<true>::allocations() << '\n';
    }

    std::cout << "--- 只读拷贝不克隆 ---\n";
    CowText<true>::reset_allocations();
    {
        CowText<true> base("shared");
        CowText<true> c1 = base;   // 只加计数
        CowText<true> c2 = c1;     // 只加计数
        std::cout << "c1=" << c1.str() << " c2=" << c2.str()
                  << " allocations=" << CowText<true>::allocations() << '\n';
    }
}
