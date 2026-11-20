#include "ring/ring.hpp"

#include <numeric>

namespace ring {

int sum(const std::vector<int>& values) {
  return std::accumulate(values.begin(), values.end(), 0);
}

std::string version() { return "0.3.0"; }

void Counter::add(int value) { value_ += value; }

int Counter::value() const { return value_; }

}  // namespace ring
