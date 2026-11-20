#include "registry.h"

#include <string>
#include <vector>

static std::vector<std::string>& units() {
  static std::vector<std::string> all;
  return all;
}

void add_unit(const char* name) {
  units().push_back(name);
}

int unit_count() {
  return static_cast<int>(units().size());
}
