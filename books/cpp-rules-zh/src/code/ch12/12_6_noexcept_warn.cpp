// 第 12.6 节：编译器能抓住直接写在 noexcept 函数里的 throw
// mode: warn
// expect: -Wexceptions
#include <cstdio>

void flush(bool broken) noexcept {
    if (broken) {
        throw 1;  // clang: -Wexceptions；GCC: -Wterminate
    }
    std::puts("flushed");
}

int main() {
    flush(false);
}
