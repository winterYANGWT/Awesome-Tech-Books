// 第 12.7 节：throw() 仍被编译器接受，并等价于 noexcept(true)
// mode: run
#include <cstdio>

void ping() throw() {}

static_assert(noexcept(ping()));

int main() {
    ping();
    std::printf("throw() accepted; noexcept(ping()) == true\n");
}
