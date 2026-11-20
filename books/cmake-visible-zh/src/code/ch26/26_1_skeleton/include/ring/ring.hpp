#pragma once

#include <string>
#include <vector>

#include "ring/ring_export.h"

namespace ring {

RING_EXPORT int sum(const std::vector<int>& values);
RING_EXPORT std::string version();

class RING_EXPORT Counter {
 public:
  void add(int value);
  int value() const;

 private:
  int value_ = 0;
};

}  // namespace ring
