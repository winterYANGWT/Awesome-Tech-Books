#include <cstdio>
#include "net/net.h"
#include "core/core.h"

#ifdef CORE_PRIVATE
#error "CORE_PRIVATE is PRIVATE to core and must not reach app"
#endif

int main()
{
    std::printf("core=%d net=%d\n", core_value(), net_value());
}
