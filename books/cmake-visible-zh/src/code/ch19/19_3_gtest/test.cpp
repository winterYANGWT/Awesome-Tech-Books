#include <gtest/gtest.h>

#include <string>

TEST(Arithmetic, Adds) {
  EXPECT_EQ(2 + 3, 5);
}

TEST(Arithmetic, Multiplies) {
  EXPECT_EQ(6 * 7, 42);
}

TEST(Text, Joins) {
  EXPECT_EQ(std::string("cm") + "ake", "cmake");
}
