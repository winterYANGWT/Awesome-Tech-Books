// 第 31.4 节：C++26 的 std::execution 尚未进入本书环境的两个标准库
// mode: fail
// expect: schedule
#include <execution>

int main() {
    auto sender = std::execution::schedule();
    (void)sender;
}
