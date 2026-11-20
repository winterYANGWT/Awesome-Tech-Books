#include <cstdio>

int main() {
#if FAST_PATH
    std::printf("fast path, log=%s\n", LOG_LEVEL);
#else
    std::printf("slow path, log=%s\n", LOG_LEVEL);
#endif
}
