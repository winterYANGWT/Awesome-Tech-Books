#include <gtest/gtest.h>

#include "ring/ring.hpp"

TEST(Ring, SumsValues) {
  EXPECT_EQ(ring::sum({1, 2, 3}), 6);
}

TEST(Ring, CounterAdds) {
  ring::Counter counter;
  counter.add(4);
  counter.add(5);
  EXPECT_EQ(counter.value(), 9);
}
