// 第 15.4 节：返回值能被 [[nodiscard]] 保护，输出参数不能
// mode: warn
// expect: nodiscard
#include <optional>
#include <string_view>

[[nodiscard]] std::optional<int> parse_port(std::string_view text) {
    if (text == "8080") {
        return 8080;
    }
    return std::nullopt;
}

void parse_port_ref(std::string_view text, int& port) {
    if (text == "8080") {
        port = 8080;
    }
}

int main() {
    int port = 0;
    parse_port_ref("8080", port);  // 没有返回值可丢，编译器不会提醒检查结果
    parse_port("8080");            // 丢掉 [[nodiscard]] 返回值，clang 会警告
}
