#include <cstdio>
#include "a/a.h"

#if __has_include("b/b.h")
#error "b include leaked into app"
#endif

int main()
{
    std::printf("a=%d\n", a_value());
}
