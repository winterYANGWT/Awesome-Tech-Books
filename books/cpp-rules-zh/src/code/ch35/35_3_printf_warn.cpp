// 第 35.3 节：printf 的格式串和实参类型对不上
// mode: warn
// expect: -Wformat
#include <cstdio>

int main() {
    const int count = 7;
    std::printf("count=%d\n", count);
    std::printf("name=%s\n", 42);  // %s 要的是 char*，这里给了 int
}
