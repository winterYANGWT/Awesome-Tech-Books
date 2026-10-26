// 第 32.4 节：用花括号包住的宏不是一条语句，放进 if/else 就会断开
// mode: fail
#include <iostream>

int trace_count = 0;
#define TRACE(msg) { std::cout << msg << '\n'; ++trace_count; }

int main() {
    const bool ok = false;
    if (ok) TRACE("ok"); else TRACE("not ok");
    std::cout << trace_count << '\n';
}
