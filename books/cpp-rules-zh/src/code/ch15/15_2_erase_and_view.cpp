// 第 15.2 节：C++20 把 erase/erase_if 放到类外，string_view 让查询不再绑定容器
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

namespace text {

// 接受 string_view：std::string、字面量、const char* 都能传，都不复制。
std::size_t count_words(std::string_view s) {
    std::size_t words = 0;
    bool in_word = false;
    for (char c : s) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            in_word = false;
        } else if (!in_word) {
            in_word = true;
            ++words;
        }
    }
    return words;
}

std::string_view trim(std::string_view s) {
    auto first = s.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    auto last = s.find_last_not_of(" \t");
    return s.substr(first, last - first + 1);
}

// 连续斜杠合并成一条：算法做扫描，最后只调一次成员 erase 收尾。
void squeeze_slashes(std::string& path) {
    auto last = std::unique(path.begin(), path.end(),
                            [](char a, char b) { return a == '/' && b == '/'; });
    path.erase(last, path.end());
}

}  // namespace text

int main() {
    std::string raw = "  GET\t/index.html  HTTP/1.1  ";
    std::string_view view = text::trim(raw);
    std::cout << "trim    = [" << view << "]\n";
    std::cout << "words   = " << text::count_words(view) << '\n';

    std::string path = "/usr//local///share//app";
    text::squeeze_slashes(path);
    std::cout << "squeeze = " << path << '\n';

    std::string letters = "a\tb\tc";
    std::erase(letters, '\t');  // C++20：非成员 erase
    std::erase_if(letters, [](char c) { return c == 'b'; });  // C++20：非成员 erase_if
    std::cout << "erase   = " << letters << '\n';

    auto it = std::ranges::find(path, 's');
    std::cout << "find(s) = " << std::distance(path.begin(), it) << '\n';

    // 同一份算法直接吃字面量，不需要先构造 std::string。
    std::cout << "literal = " << text::count_words("two words") << '\n';
}
