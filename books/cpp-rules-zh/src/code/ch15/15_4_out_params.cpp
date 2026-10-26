// 第 15.4 节：输出参数让调用点看不出实参被改写
#include <charconv>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace config {

// 非 const 引用做输出参数：调用点 parse_port_ref(line, port) 看起来像传值。
bool parse_port_ref(std::string_view text, int& port) {
    auto eq = text.find('=');
    if (eq == std::string_view::npos) {
        return false;
    }
    auto digits = text.substr(eq + 1);
    int value = 0;
    auto [ptr, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), value);
    if (ec != std::errc{} || ptr != digits.data() + digits.size()
        || value < 0 || value > 65535) {
        return false;
    }
    port = value;
    return true;
}

// 指针版本：调用点必须写 &port，读者一眼知道这个实参可能被改。
bool parse_port_ptr(std::string_view text, int* port) {
    if (port == nullptr) {
        return false;
    }
    return parse_port_ref(text, *port);
}

// 返回值版本：数据流从签名就看得到，失败用 nullopt 表示。
std::optional<int> parse_port(std::string_view text) {
    int value = 0;
    if (!parse_port_ref(text, value)) {
        return std::nullopt;
    }
    return value;
}

// 两个同类型输出参数，调用时写反了也能编译。
bool split_pair(std::string_view text, std::string& left, std::string& right) {
    auto colon = text.find(':');
    if (colon == std::string_view::npos) {
        return false;
    }
    left.assign(text.substr(0, colon));
    right.assign(text.substr(colon + 1));
    return true;
}

}  // namespace config

int main() {
    int port = 443;  // 上一次留下来的旧值
    bool ok = config::parse_port_ref("port=8080", port);
    std::cout << "ref:      ok=" << ok << " port=" << port << '\n';

    port = 443;
    ok = config::parse_port_ref("port=oops", port);
    std::cout << "ref 失败: ok=" << ok << " port=" << port << "（旧值没变，调用点看不出来）\n";

    port = 443;
    ok = config::parse_port_ptr("port=8080", &port);
    std::cout << "ptr:      ok=" << ok << " port=" << port << '\n';

    auto maybe = config::parse_port("port=8080");
    std::cout << "optional: " << (maybe ? std::to_string(*maybe) : "none") << '\n';

    auto bad = config::parse_port("port=oops");
    std::cout << "optional 失败: " << (bad ? std::to_string(*bad) : "none") << '\n';

    std::string host;
    std::string service;
    // 参数顺序是 (text, left, right)，这里故意把 host 和 service 传反。
    config::split_pair("db:5432", service, host);
    std::cout << "split_pair 写反: host=" << host << " service=" << service << '\n';
}
