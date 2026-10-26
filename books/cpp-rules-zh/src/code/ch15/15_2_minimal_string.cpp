// 第 15.2 节：把 string 的整族操作外移，只留 insert 和 erase 两个原语
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace text {

inline constexpr std::size_t npos = std::string::npos;

class Text {
public:
    Text() = default;
    explicit Text(std::string value) : buf_(std::move(value)) {}
    Text(const char* s, std::size_t n) : buf_(s, n) {}

    Text(const Text&) = default;
    Text& operator=(const Text&) = default;

    std::size_t size() const { return buf_.size(); }
    const char* data() const { return buf_.data(); }
    char operator[](std::size_t i) const { return buf_[i]; }

    // 只留两个碰内部表示的原语：按位置插入、按位置删除。
    Text& insert(std::size_t pos, const char* s, std::size_t n) {
        buf_.insert(pos, s, n);
        return *this;
    }

    Text& erase(std::size_t pos, std::size_t n) {
        buf_.erase(pos, n);
        return *this;
    }

    void swap(Text& other) noexcept { buf_.swap(other.buf_); }

private:
    std::string buf_;
};

bool empty(const Text& t) { return t.size() == 0; }

char at(const Text& t, std::size_t pos) {
    if (pos >= t.size()) {
        throw std::out_of_range("text::at");
    }
    return t[pos];
}

// append 全族都归约到“在末尾 insert”，没有一个是成员。
Text& append(Text& t, const Text& suffix) {
    return t.insert(t.size(), suffix.data(), suffix.size());
}

Text& append(Text& t, const char* s) {
    return t.insert(t.size(), s, std::char_traits<char>::length(s));
}

Text& append(Text& t, char c) { return t.insert(t.size(), &c, 1); }

Text& push_back(Text& t, char c) { return append(t, c); }

// += 在语法上属于赋值运算符，但语言只强制 operator= 本身是成员。
Text& operator+=(Text& lhs, const Text& rhs) { return append(lhs, rhs); }
Text& operator+=(Text& lhs, const char* rhs) { return append(lhs, rhs); }

Text& assign(Text& t, const Text& other) {
    Text tmp(other);
    t.swap(tmp);
    return t;
}

Text substr(const Text& t, std::size_t pos, std::size_t n) {
    if (pos > t.size()) {
        throw std::out_of_range("text::substr");
    }
    return Text(t.data() + pos, std::min(n, t.size() - pos));
}

int compare(const Text& a, const Text& b) {
    return std::string_view(a.data(), a.size())
        .compare(std::string_view(b.data(), b.size()));
}

std::size_t find(const Text& haystack, const Text& needle) {
    auto pos = std::string_view(haystack.data(), haystack.size())
                   .find(std::string_view(needle.data(), needle.size()));
    return pos == std::string_view::npos ? npos : pos;
}

Text operator+(Text lhs, const Text& rhs) {
    lhs += rhs;
    return lhs;
}

}  // namespace text

int main() {
    text::Text greeting("hello");
    text::append(greeting, " world");
    greeting += text::Text("!");   // 非成员 operator+=，靠 ADL 找到
    text::push_back(greeting, '?');
    std::cout << "append/+=/push_back: " << std::string(greeting.data(), greeting.size()) << '\n';

    greeting.erase(5, 1);
    std::cout << "erase(5,1):          " << std::string(greeting.data(), greeting.size()) << '\n';

    text::Text copy("xxxxx");
    text::assign(copy, greeting);
    std::cout << "assign:              " << std::string(copy.data(), copy.size()) << '\n';
    std::cout << "compare(copy,greet): " << text::compare(copy, greeting) << '\n';

    text::Text part = text::substr(greeting, 5, 5);
    std::cout << "substr(5,5):         " << std::string(part.data(), part.size()) << '\n';
    std::cout << "find(\"world\"):        " << text::find(greeting, text::Text("world")) << '\n';
    std::cout << "at(1):               " << text::at(greeting, 1) << '\n';
    std::cout << "empty(greeting):     " << std::boolalpha << text::empty(greeting) << '\n';

    text::Text joined = greeting + text::Text("!");
    std::cout << "operator+:           " << std::string(joined.data(), joined.size()) << '\n';
}
