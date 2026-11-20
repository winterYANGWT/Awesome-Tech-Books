#include <cstdio>
#include "units/units.h"

int main()
{
    std::printf("units=%d cplusplus=%ld\n", units_value(), __cplusplus);
}
