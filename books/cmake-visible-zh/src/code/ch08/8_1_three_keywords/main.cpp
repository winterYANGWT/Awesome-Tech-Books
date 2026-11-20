#include <cstdio>
#include "calc.h"

#ifndef CALC_INTERFACE
#error "CALC_INTERFACE should reach the consumer"
#endif
#ifndef CALC_PUBLIC
#error "CALC_PUBLIC should reach the consumer"
#endif
#ifdef CALC_PRIVATE
#error "CALC_PRIVATE must not reach the consumer"
#endif

int main()
{
    std::printf("calc=%d interface=%d\n", calc_value(), CALC_INTERFACE);
}
