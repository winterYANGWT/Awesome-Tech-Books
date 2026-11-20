#include <cstdio>

int main() {
#if defined(__clang__)
    std::puts("clang");
#elif defined(__GNUC__)
    std::puts("gcc");
#else
    std::puts("other");
#endif
}
