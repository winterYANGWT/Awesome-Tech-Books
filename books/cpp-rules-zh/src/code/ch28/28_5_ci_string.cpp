// 第 28.5 节：用自定义 char_traits 做大小写不敏感的字符串，并为边界补适配
#include <cctype>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>
#include <type_traits>

struct ci_char_traits : std::char_traits<char> {
    static unsigned char fold(char c) {
        return static_cast<unsigned char>(std::toupper(static_cast<unsigned char>(c)));
    }

    static bool eq(char a, char b) { return fold(a) == fold(b); }
    static bool lt(char a, char b) { return fold(a) < fold(b); }

    static int compare(const char* a, const char* b, std::size_t n) {
        for (std::size_t i = 0; i < n; ++i) {
            const unsigned char ca = fold(a[i]);
            const unsigned char cb = fold(b[i]);
            if (ca < cb) {
                return -1;
            }
            if (ca > cb) {
                return 1;
            }
        }
        return 0;
    }

    static const char* find(const char* s, std::size_t n, const char& a) {
        for (std::size_t i = 0; i < n; ++i) {
            if (eq(s[i], a)) {
                return s + i;
            }
        }
        return nullptr;
    }
};

using ci_string = std::basic_string<char, ci_char_traits>;

// ci_string 与 cout 的 char/traits 不同，标准库没有现成的 operator<<。
std::ostream& operator<<(std::ostream& os, const ci_string& s) {
    return os << s.c_str();
}

int main() {
    static_assert(!std::is_same_v<ci_string, std::string>);

    const ci_string name = "AbCdE";
    std::cout << std::boolalpha;
    std::cout << "name == \"abcde\": " << (name == "abcde") << '\n';
    std::cout << "name == \"ABCDE\": " << (name == "ABCDE") << '\n';
    std::cout << "name <  \"abCdf\": " << (name < ci_string("abCdf")) << '\n';
    std::cout << "name.find(\"cd\") = " << name.find("cd") << '\n';
    std::cout << "原样保存的 c_str(): " << name.c_str() << '\n';

    // 与普通 string 混用时没有统一的 traits，必须显式转换。
    const std::string prefix = "user:";
    const std::string combined = prefix + name.c_str();
    std::cout << "拼接后: " << combined << '\n';
}
