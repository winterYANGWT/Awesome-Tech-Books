#include <gtest/gtest.h>
#include <zlib.h>

#include <cstdio>

TEST(Zlib, Version) { EXPECT_NE(zlibVersion(), nullptr); }

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  std::printf("zlib %s\n", zlibVersion());
  return RUN_ALL_TESTS();
}
