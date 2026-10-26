// 第 3.2 节：不同访问控制块之间没有布局保证，offsetof 的前提不成立
// mode: warn
// expect: -Winvalid-offsetof
#include <cstddef>
#include <cstdio>

class Split {
public:
    int first;
private:
    [[maybe_unused]] int second;
public:
    int third;
};

int main() {
    // 不是标准布局类型，offsetof 只是条件支持；这里让编译器把问题指出来
    std::printf("third at %zu\n", offsetof(Split, third));
}
