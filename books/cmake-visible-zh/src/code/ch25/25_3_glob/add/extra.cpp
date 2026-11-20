#include "registry.h"

namespace {
struct Extra {
  Extra() { add_unit("extra"); }
} extra;
}  // namespace
