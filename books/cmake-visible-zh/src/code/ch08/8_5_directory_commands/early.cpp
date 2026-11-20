#include <cstdio>
#include "common.h"

int main()
{
#ifdef OPT_SEEN
    std::printf("early opt=%d\n", OPT_SEEN);
#else
    std::puts("early opt=none");
#endif
#ifdef DEF_SEEN
    std::printf("early def=%d common=%d\n", DEF_SEEN, common_value());
#else
    std::puts("early def=none");
#endif
}
