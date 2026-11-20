#include <zlib.h>

int util_value();

int core_value() { return util_value() + static_cast<int>(zlibVersion()[0] - '0'); }
