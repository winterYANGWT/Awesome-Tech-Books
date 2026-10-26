// 第 24.7 节：emplace_back 直接构造省掉临时对象，但在中间插入不一定更快
#include <iostream>
#include <string>
#include <vector>

struct Label {
    std::string text;
    static inline int direct = 0;
    static inline int copies = 0;
    static inline int moves = 0;
    static inline int move_assigns = 0;

    explicit Label(const char* t) : text(t) { ++direct; }
    Label(const Label& other) : text(other.text) { ++copies; }
    Label(Label&& other) noexcept : text(std::move(other.text)) { ++moves; }
    Label& operator=(Label&& other) noexcept {
        text = std::move(other.text);
        ++move_assigns;
        return *this;
    }
};

void reset() {
    Label::direct = Label::copies = Label::moves = Label::move_assigns = 0;
}

void report(const char* how) {
    std::cout << how << ": 直接构造 " << Label::direct
              << "，拷贝构造 " << Label::copies
              << "，移动构造 " << Label::moves
              << "，移动赋值 " << Label::move_assigns << '\n';
}

int main() {
    // push_back 需要先有一个 Label 对象，再把它移进容器
    reset();
    {
        std::vector<Label> labels;
        labels.reserve(4);
        labels.push_back(Label("alpha"));
    }
    report("push_back(Label(\"alpha\"))");

    // emplace_back 把 const char* 直接转给 Label 的构造函数
    reset();
    {
        std::vector<Label> labels;
        labels.reserve(4);
        labels.emplace_back("alpha");
    }
    report("emplace_back(\"alpha\")");

    // 在中间插入：vector 要把后面的元素挪开，优势没有那么明显
    std::vector<Label> middle;
    middle.reserve(4);
    middle.emplace_back("a");
    middle.emplace_back("b");
    middle.emplace_back("c");

    reset();
    middle.insert(middle.begin(), Label("front"));
    report("insert(begin(), Label(\"front\"))");

    reset();
    middle.emplace(middle.begin(), "front");
    report("emplace(begin(), \"front\")");

    std::cout << "最终顺序: ";
    for (const auto& label : middle) {
        std::cout << label.text << ' ';
    }
    std::cout << '\n';
}
