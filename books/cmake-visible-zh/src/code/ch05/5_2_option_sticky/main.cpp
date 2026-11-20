#include <cstdio>

int main() {
#if FAST_PATH
    std::puts("fast path");
#else
    std::puts("slow path");
#endif
}
