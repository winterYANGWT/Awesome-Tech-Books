#include "x.h"

inline int detail_scale(int v) { return v * 2; }

extern "C" int x_extra() { return 7; }

extern "C" int x_value() { return detail_scale(21); }
