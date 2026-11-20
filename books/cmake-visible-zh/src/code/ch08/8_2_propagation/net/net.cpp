#include "net/net.h"
#include "core/core.h"

#ifndef NET_PRIVATE
#error "NET_PRIVATE should reach net.cpp"
#endif

int net_value()
{
    return core_value() + 1;
}
