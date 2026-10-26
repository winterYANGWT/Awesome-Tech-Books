// 第 7.1 节：sink 形参按值还是按引用，要看“构造进目标”还是“赋值进目标”
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

struct Label {
    static int copies;
    static int moves;
    std::string text;

    explicit Label(std::string value) : text(std::move(value)) {}
    Label(const Label& other) : text(other.text) { ++copies; }
    Label(Label&& other) noexcept : text(std::move(other.text)) { ++moves; }
    Label& operator=(const Label&) = default;
    Label& operator=(Label&&) noexcept = default;
};
int Label::copies = 0;
int Label::moves = 0;

class Shelf {
public:
    Shelf() { labels_.reserve(16); }

    void add_ref(const Label& label) { labels_.push_back(label); }
    void add_ref(Label&& label) { labels_.push_back(std::move(label)); }
    void add_value(Label label) { labels_.push_back(std::move(label)); }

    std::size_t size() const { return labels_.size(); }

private:
    std::vector<Label> labels_;
};

void reset_labels() {
    Label::copies = 0;
    Label::moves = 0;
}

class Buffer {
public:
    static int copies;
    static int moves;
    static int allocations;
    static int releases;

    explicit Buffer(std::string_view text) : len_(text.size()), cap_(text.size()) {
        if (cap_ != 0) {
            data_ = std::make_unique<char[]>(cap_);
            ++allocations;
            std::copy_n(text.data(), len_, data_.get());
        }
    }

    Buffer(const Buffer& other) : len_(other.len_), cap_(other.len_) {
        if (cap_ != 0) {
            data_ = std::make_unique<char[]>(cap_);
            ++allocations;
            std::copy_n(other.data_.get(), len_, data_.get());
        }
        ++copies;
    }

    Buffer(Buffer&& other) noexcept
        : data_(std::move(other.data_)), len_(other.len_), cap_(other.cap_) {
        other.len_ = other.cap_ = 0;
        ++moves;
    }

    Buffer& operator=(const Buffer& other) {
        if (cap_ < other.len_) {
            if (data_) {
                ++releases;
            }
            data_ = std::make_unique<char[]>(other.len_);
            cap_ = other.len_;
            ++allocations;
        }
        len_ = other.len_;
        if (len_ != 0) {
            std::copy_n(other.data_.get(), len_, data_.get());
        }
        ++copies;
        return *this;
    }

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            if (data_) {
                ++releases;
            }
            data_ = std::move(other.data_);
            len_ = other.len_;
            cap_ = other.cap_;
            other.len_ = other.cap_ = 0;
            ++moves;
        }
        return *this;
    }

    ~Buffer() {
        if (data_) {
            ++releases;
        }
    }

    std::size_t size() const { return len_; }

private:
    std::unique_ptr<char[]> data_;
    std::size_t len_ = 0;
    std::size_t cap_ = 0;
};
int Buffer::copies = 0;
int Buffer::moves = 0;
int Buffer::allocations = 0;
int Buffer::releases = 0;

class Document {
public:
    explicit Document(std::string_view body) : body_(body) {}

    void retitle_value(Buffer text) { body_ = std::move(text); }
    void retitle_ref(const Buffer& text) { body_ = text; }
    std::size_t size() const { return body_.size(); }

private:
    Buffer body_;
};

void reset_buffer() {
    Buffer::copies = 0;
    Buffer::moves = 0;
    Buffer::allocations = 0;
    Buffer::releases = 0;
}

int main() {
    Shelf shelf;
    shelf.add_ref(Label("reserved"));

    Label first("first");
    reset_labels();
    shelf.add_ref(first);
    std::cout << "重载 const&：左值 拷贝 " << Label::copies
              << "，移动 " << Label::moves << "\n";

    Label second("second");
    reset_labels();
    shelf.add_ref(std::move(second));
    std::cout << "重载 &&：右值 拷贝 " << Label::copies
              << "，移动 " << Label::moves << "\n";

    Label third("third");
    reset_labels();
    shelf.add_value(third);
    std::cout << "按值：左值 拷贝 " << Label::copies
              << "，移动 " << Label::moves << "\n";

    Label fourth("fourth");
    reset_labels();
    shelf.add_value(std::move(fourth));
    std::cout << "按值：将亡值 拷贝 " << Label::copies
              << "，移动 " << Label::moves << "\n";

    reset_labels();
    shelf.add_value(Label("fifth"));
    std::cout << "按值：纯右值 拷贝 " << Label::copies
              << "，移动 " << Label::moves << "\n";

    std::cout << "货架共 " << shelf.size() << " 个标签\n";

    // 形参通过“赋值”落进成员时，按值会先复制一份，再把旧容量释放掉。
    Buffer replacement("short-title");
    Document by_value("a-much-longer-original-title-with-capacity");
    reset_buffer();
    by_value.retitle_value(replacement);
    std::cout << "按值 + move：拷贝 " << Buffer::copies << "，移动 " << Buffer::moves
              << "，分配 " << Buffer::allocations << "，释放 " << Buffer::releases << "\n";

    Document by_ref("a-much-longer-original-title-with-capacity");
    reset_buffer();
    by_ref.retitle_ref(replacement);
    std::cout << "const& 赋值：拷贝 " << Buffer::copies << "，移动 " << Buffer::moves
              << "，分配 " << Buffer::allocations << "，释放 " << Buffer::releases << "\n";

    std::cout << "两份文档长度：" << by_value.size() << " / " << by_ref.size() << "\n";
}
