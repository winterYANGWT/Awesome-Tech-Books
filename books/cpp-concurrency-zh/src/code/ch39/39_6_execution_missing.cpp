// mode: fail
// expect: just
// C++26 标准有 sender/receiver，但 libstdc++ 15 还没有实现 just。
#include <execution>

int main() {
    auto s = std::execution::just(42);
    (void)s;
}
