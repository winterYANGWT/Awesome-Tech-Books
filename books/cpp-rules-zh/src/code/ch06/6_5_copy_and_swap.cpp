// 第 6.5 节：copy-and-swap、noexcept 的 swap 与强异常安全
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace imaging {

class Image {
public:
    Image(std::string name, std::vector<std::uint8_t> pixels)
        : name_(std::move(name)), pixels_(std::move(pixels)) {}

    // 成员 swap 只交换内部状态，声明 noexcept 是 copy-and-swap 的基础。
    void swap(Image& other) noexcept {
        using std::swap;
        swap(name_, other.name_);
        swap(pixels_, other.pixels_);
        ++custom_swaps;
    }

    Image(const Image&) = default;
    Image(Image&&) noexcept = default;
    Image& operator=(const Image& rhs) {
        Image temp(rhs);   // 所有可能抛的工作都在这里
        swap(temp);        // 提交阶段只做不抛的交换
        return *this;
    }
    Image& operator=(Image&&) noexcept = default;

    const std::string& name() const { return name_; }
    std::size_t size() const { return pixels_.size(); }

    static inline int custom_swaps = 0;

private:
    std::string name_;
    std::vector<std::uint8_t> pixels_;
};

// 非成员 swap 与类型放在同一命名空间，ADL 才能找到它。
void swap(Image& lhs, Image& rhs) noexcept {
    lhs.swap(rhs);
}

}  // namespace imaging

// 一个会在复制时抛异常的小类型，用来验证“失败时目标不变”。
struct ThrowingPayload {
    static inline bool armed = false;
    int id = 0;

    ThrowingPayload() = default;
    explicit ThrowingPayload(int value) : id(value) {}
    ThrowingPayload(const ThrowingPayload& rhs) : id(rhs.id) {
        if (armed) {
            throw std::runtime_error("payload copy failed");
        }
    }
    ThrowingPayload& operator=(const ThrowingPayload&) = default;
};

class Document {
public:
    Document(std::string title, int version)
        : title_(std::move(title)), version_(version) {}

    void swap(Document& other) noexcept {
        using std::swap;
        swap(title_, other.title_);
        swap(version_, other.version_);
        swap(payload_.id, other.payload_.id);
    }

    Document(const Document&) = default;
    Document& operator=(const Document& rhs) {
        Document temp(rhs);
        swap(temp);
        return *this;
    }

    const std::string& title() const { return title_; }
    int version() const { return version_; }

private:
    std::string title_;
    int version_ = 0;
    ThrowingPayload payload_;
};

void swap(Document& lhs, Document& rhs) noexcept {
    lhs.swap(rhs);
}

template <typename T>
void exchange(T& lhs, T& rhs) {
    using std::swap;   // 先让 std::swap 可见
    swap(lhs, rhs);    // 不加限定：ADL 优先找到类型专用版本
}

static_assert(std::is_nothrow_swappable_v<imaging::Image>);
static_assert(std::is_nothrow_swappable_v<Document>);

int main() {
    imaging::Image first("first", {1, 2, 3});
    imaging::Image second("second", {9});

    exchange(first, second);
    std::cout << "ADL: first=" << first.name() << "(" << first.size() << "), "
              << "second=" << second.name() << "(" << second.size() << "), "
              << "custom_swaps=" << imaging::Image::custom_swaps << '\n';

    std::swap(first, second);   // 限定调用，绕过 ADL
    std::cout << "std::swap 之后 custom_swaps=" << imaging::Image::custom_swaps
              << ", first=" << first.name() << '\n';

    Document stable("stable", 1);
    Document incoming("incoming", 2);
    ThrowingPayload::armed = true;
    try {
        stable = incoming;
    } catch (const std::exception& e) {
        std::cout << "捕获: " << e.what() << '\n';
    }
    ThrowingPayload::armed = false;
    std::cout << "强保证: stable=" << stable.title()
              << " version=" << stable.version() << '\n';
}
