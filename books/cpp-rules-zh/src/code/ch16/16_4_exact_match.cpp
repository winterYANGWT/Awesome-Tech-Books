// 第 16.4 节：为常见实参补精确匹配的重载，省掉临时 Text
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

class Text {
public:
    // 故意不加 explicit，模拟“允许从 const char* 隐式构造”的字符串类型
    Text(const char* text) : text_(text) { ++temporaries_; }
    explicit Text(std::string text) : text_(std::move(text)) {}

    std::string_view view() const { return text_; }
    static int temporaries() { return temporaries_; }

private:
    std::string text_;
    static inline int temporaries_ = 0;
};

// 只有通用签名：调用点必须先把 "hello" 转成临时 Text
bool same(const Text& lhs, const Text& rhs) {
    return lhs.view() == rhs.view();
}

bool operator==(const Text& lhs, const Text& rhs) {
    return lhs.view() == rhs.view();
}

// 精确匹配的补充版本：右侧/左侧的 const char* 不再构造 Text
bool operator==(const Text& lhs, const char* rhs) {
    return lhs.view() == rhs;
}

bool operator==(const char* lhs, const Text& rhs) {
    return lhs == rhs.view();
}

int main() {
    Text text("hello");

    int before_generic = Text::temporaries();
    bool generic = same(text, "hello");
    int after_generic = Text::temporaries();

    bool exact_right = (text == "hello");
    int after_right = Text::temporaries();

    bool exact_left = ("hello" == text);
    int after_left = Text::temporaries();

    std::cout << std::boolalpha;
    std::cout << "通用签名 " << generic << "，构造 Text " << after_generic - before_generic << " 个\n";
    std::cout << "text == \"hello\" " << exact_right << "，构造 Text " << after_right - after_generic << " 个\n";
    std::cout << "\"hello\" == text " << exact_left << "，构造 Text " << after_left - after_right << " 个\n";
}
