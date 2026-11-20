#include "calc.h"

#ifndef CALC_PRIVATE
#error "CALC_PRIVATE should reach calc.cpp"
#endif

int calc_value()
{
#ifdef CALC_PUBLIC
    return CALC_PUBLIC;
#else
    return 0;
#endif
}
