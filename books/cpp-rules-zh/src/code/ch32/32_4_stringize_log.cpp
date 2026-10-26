// 第 32.4 节：字符串化宏把表达式原文带进日志，位置交给 source_location
#include <iostream>
#include <source_location>
#include <string_view>

// 两层宏：外层先展开实参里的宏，内层再做字符串化。
#define STRINGIZE_IMPL(x) #x
#define STRINGIZE(x) STRINGIZE_IMPL(x)
#define LIMIT 10

// do-while(0) 把多条语句包成一条语句；实参先存进 value，只求值一次。
#define LOG_EXPR(expr)                                  \
    do {                                                \
        const auto value = (expr);                      \
        std::cout << #expr << " = " << value << '\n';   \
    } while (0)

void log_at(std::string_view what, int value,
            const std::source_location& loc = std::source_location::current()) {
    std::cout << loc.function_name() << ':' << loc.line()
              << " " << what << " = " << value << '\n';
}

int next_calls = 0;
int next() {
    return ++next_calls;
}

int main() {
    const int a = 1, b = 2;

    LOG_EXPR(a + b);      // 表达式原文是 "a + b"
    LOG_EXPR(next());     // 只调用 next 一次
    LOG_EXPR(LIMIT);      // #expr 不展开 LIMIT，打印的是宏名
    LOG_EXPR(a  +   b);   // 中间的空白被压成一个空格

    std::cout << "STRINGIZE(LIMIT) = " << STRINGIZE(LIMIT) << '\n';
    log_at("a + b", a + b);
    std::cout << "next 调用 " << next_calls << " 次\n";
}
