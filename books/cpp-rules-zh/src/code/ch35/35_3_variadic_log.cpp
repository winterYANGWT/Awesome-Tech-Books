// 第 35.3 节：用可变参数模板和 std::format 取代省略号
#include <format>
#include <iostream>
#include <string>

template <class... Args>
void log_line(const Args&... args) {
    std::cout << "[log]";
    ((std::cout << ' ' << args), ...);  // C++17 折叠表达式，类型逐个已知
    std::cout << '\n';
}

int main() {
    log_line("sensor", 7, 21.5);
    std::cout << std::format("{} + {} = {}\n", 2, 3, 5);
}
