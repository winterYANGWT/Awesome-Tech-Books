// 第 12.6 节：异常从 noexcept 函数逃出时直接 terminate
// mode: crash
// expect: before
#include <cstdio>

void may_throw() { throw 42; }

// 编译器只看到一次普通函数调用，不会跨函数分析它是否抛出
void commit() noexcept { may_throw(); }

int main() {
    std::puts("before");
    commit();
    std::puts("after");
}
