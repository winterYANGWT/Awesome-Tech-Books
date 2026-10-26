// 第 28.3 节：to_chars、format 与 snprintf 的长度/类型行为对照
#include <array>
#include <charconv>
#include <cstdio>
#include <format>
#include <iostream>
#include <string>
#include <system_error>

int main() {
    // to_chars：写进调用方给的缓冲区，不分配、不带 '\0'，用返回的指针定长度。
    std::array<char, 32> digits{};
    const int orderId = 12345;
    const auto [end, ec] = std::to_chars(digits.data(), digits.data() + digits.size(), orderId);
    if (ec == std::errc{}) {
        std::cout << "to_chars: " << std::string(digits.data(), end) << '\n';
    }

    // format：格式串与实参类型在编译期配对，返回一个自动管理长度的 string。
    const std::string line = std::format("{:>8}|{:<6}|{:04d}", "id", 42, 7);
    std::cout << "format:   [" << line << "]\n";
    std::cout << "浮点:     " << std::format("{:.2f}", 3.14159) << '\n';

    // snprintf：长度受检，但只保证不越界；放不下就截断，返回值仍是完整长度。
    char small[8]{};
    const int needed = std::snprintf(small, sizeof small, "%d", 123456789);
    std::cout << "snprintf 返回需要的长度=" << needed
              << "，缓冲区里是 \"" << small << "\"\n";
}
