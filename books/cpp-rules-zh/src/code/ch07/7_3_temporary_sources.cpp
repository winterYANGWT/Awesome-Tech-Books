// 第 7.3 节：临时对象主要来自隐式转换和按值返回
#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

struct Text {
    static int constructions;
    std::string value;

    Text(const char* s) : value(s) { ++constructions; }  // 隐式转换会用到
    explicit Text(std::string s) : value(std::move(s)) { ++constructions; }
};
int Text::constructions = 0;

std::size_t count_char(const Text& text, char ch) {
    std::size_t n = 0;
    for (char c : text.value) {
        if (c == ch) {
            ++n;
        }
    }
    return n;
}

std::size_t count_char_view(std::string_view text, char ch) {
    std::size_t n = 0;
    for (char c : text) {
        if (c == ch) {
            ++n;
        }
    }
    return n;
}

Text make_title() {
    return Text("inventory");
}

int main() {
    char buffer[] = "banana";

    Text::constructions = 0;
    std::size_t old_way = count_char(buffer, 'a');
    std::cout << "const Text& 接 char[]：构造 Text " << Text::constructions
              << " 次，找到 " << old_way << " 个 a\n";

    Text::constructions = 0;
    std::size_t new_way = count_char_view(buffer, 'a');
    std::cout << "string_view 接 char[]：构造 Text " << Text::constructions
              << " 次，找到 " << new_way << " 个 a\n";

    // 按值返回：返回值本身就是没有名字的临时对象。
    Text::constructions = 0;
    Text title = make_title();
    std::cout << "按值返回：Text 构造 " << Text::constructions
              << " 次，内容 " << title.value << "\n";
}
