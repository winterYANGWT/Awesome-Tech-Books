#include "registry.h"

namespace {
struct Third {
  Third() { add_unit("third"); }
} third;
}  // namespace
