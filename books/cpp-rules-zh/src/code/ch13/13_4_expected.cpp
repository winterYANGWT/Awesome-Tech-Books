// 第 13.4 节：expected 同时携带值和失败原因，调用方不必先声明输出参数
// std: c++23
#include <charconv>
#include <expected>
#include <iostream>
#include <string_view>
#include <system_error>

enum class ParseError {
    empty,
    not_a_number,
    out_of_range,
};

std::string_view describe(ParseError error) {
    switch (error) {
    case ParseError::empty: return "输入为空";
    case ParseError::not_a_number: return "不是十进制整数";
    case ParseError::out_of_range: return "端口超出 1..65535";
    }
    return "未知错误";
}

std::expected<int, ParseError> parse_port(std::string_view text) {
    if (text.empty()) {
        return std::unexpected(ParseError::empty);
    }

    int value = 0;
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto [ptr, error] = std::from_chars(begin, end, value);
    if (error == std::errc::invalid_argument) {
        return std::unexpected(ParseError::not_a_number);
    }
    if (error == std::errc::result_out_of_range) {
        return std::unexpected(ParseError::out_of_range);
    }
    if (ptr != end) {
        return std::unexpected(ParseError::not_a_number);
    }
    if (value < 1 || value > 65535) {
        return std::unexpected(ParseError::out_of_range);
    }
    return value;
}

int main() {
    const std::string_view inputs[] = {"8080", "0", "abc", "70000", ""};
    for (std::string_view text : inputs) {
        const auto result = parse_port(text);
        if (result) {
            std::cout << "'" << text << "' -> port " << *result << "\n";
        } else {
            std::cout << "'" << text << "' -> " << describe(result.error()) << "\n";
        }
    }

    // 值存在时才执行转换；错误原样留在 expected 里。
    const auto doubled = parse_port("21").transform([](int port) { return port * 2; });
    std::cout << "21 的两倍：" << (doubled ? *doubled : -1) << "\n";
}
