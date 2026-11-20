#include <cstdio>

#include "config.h"

int main() {
#if WANT_FAST
    std::puts("运行：快速路径 (WANT_FAST=ON)");
#else
    std::puts("运行：普通路径 (WANT_FAST=OFF)");
#endif
}
