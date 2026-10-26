// 第 28.5 节：改了 traits 的字符串不是 std::string，不能直接写给 cout
// mode: fail
// expect: invalid operands
#include <cctype>
#include <cstddef>
#include <iostream>
#include <string>

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
            if (ca != cb) {
                return ca < cb ? -1 : 1;
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

int main() {
    const ci_string name = "AbCdE";
    std::cout << name << '\n';  // 没有匹配的 operator<<
}
