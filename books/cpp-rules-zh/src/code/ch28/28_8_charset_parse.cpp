// 第 28.8 节：逐字符读流、atoi/from_chars、字符分类与字面量的只读类型
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

void parse_with_from_chars(std::string_view text) {
    int value = 0;
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);

    std::cout << "from_chars(\"" << text << "\"): ";
    if (ec == std::errc{}) {
        std::cout << "值=" << value << "，消费完整=" << (ptr == end ? "是" : "否");
    } else if (ec == std::errc::invalid_argument) {
        std::cout << "没有数字";
    } else if (ec == std::errc::result_out_of_range) {
        std::cout << "超出 int 范围";
    }
    std::cout << '\n';
}

int main() {
    // 只有数字字符保证连续；字母的编码顺序不能假设。
    static_assert('9' - '0' == 9);

    // 字符串字面量的类型是 const char[N]，不是可写的 char*。
    static_assert(std::is_array_v<std::remove_reference_t<decltype("hello")>>);
    static_assert(std::is_const_v<std::remove_reference_t<decltype("hello")>>);

    const std::string source = "  ab\ncd  ";
    std::istringstream rawIn(source);
    const std::string raw{std::istreambuf_iterator<char>(rawIn),
                          std::istreambuf_iterator<char>()};

    std::istringstream tokenIn(source);
    std::istream_iterator<std::string> tokenIt(tokenIn);
    const std::istream_iterator<std::string> end;
    int tokens = 0;
    for (; tokenIt != end; ++tokenIt) {
        ++tokens;
    }

    std::cout << "istreambuf_iterator 读到 " << raw.size() << " 个字符，原样保留="
              << (raw == source ? "是" : "否") << '\n';
    std::cout << "istream_iterator 读到 " << tokens << " 个单词，空白被跳过\n";

    // atoi 分不清“值就是 0”和“解析失败”。
    std::cout << "atoi(\"0\")=" << std::atoi("0")
              << "，atoi(\"abc\")=" << std::atoi("abc")
              << "，atoi(\"\")=" << std::atoi("") << '\n';

    parse_with_from_chars("0");
    parse_with_from_chars("abc");
    parse_with_from_chars("12x");
    parse_with_from_chars("999999999999999999999");

    // 字符分类和大小写转换先把 char 转成 unsigned char，避免负值传进 ctype。
    for (char c : std::string("7aA")) {
        const unsigned char u = static_cast<unsigned char>(c);
        std::cout << '\'' << c << "': isdigit=" << (std::isdigit(u) != 0)
                  << " isalpha=" << (std::isalpha(u) != 0)
                  << " upper=" << static_cast<char>(std::toupper(u)) << '\n';
    }
}
