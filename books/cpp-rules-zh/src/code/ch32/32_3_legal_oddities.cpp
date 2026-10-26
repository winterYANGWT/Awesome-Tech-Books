// 第 32.3 节：看着像笔误的代码，按翻译阶段规则其实全都合法
#include <iostream>

// %: 是 # 的 digraph，所以这一行就是 #define
%:define LIMIT 3

struct Packet {
    static bool accept(const char* p) {
        // p[0] 非空，且 p[1] 不大于 p[2]；:> 是 ] 的 digraph
        return p && 0[p] != '\0' and not (p[1:>>p[2]);
    };
};
;   // 命名空间作用域的空声明，C++11 起明确合法

int main() {
    const char buf[] = {'A', 'B', 'C', '\0'};
    int a<:LIMIT:> = {1, 2, 3};   // int a[3]
    std::cout << std::boolalpha << Packet::accept(buf) << ' '
              << a<:1:> << ' ' << a<:2:> << '\n';
}
