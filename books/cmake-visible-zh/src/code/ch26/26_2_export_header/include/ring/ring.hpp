#pragma once

#include "ring/ring_export.h"

namespace ring {

RING_EXPORT int visible(int value);

// Deliberately not marked with RING_EXPORT.
int internal(int value);

}  // namespace ring
