#include <gtest/gtest.h>
#include <zlib.h>

TEST(DebugFind, Version) { EXPECT_STRNE("", zlibVersion()); }
