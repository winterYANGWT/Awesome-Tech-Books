#pragma once

#if __cplusplus < 201703L
#error "units needs C++17"
#endif

inline int units_value()
{
    return UNITS_LEVEL;
}
