#include <cstdio>

#include <gtest/gtest.h>

int main() {
#ifdef FAKE_GTEST
  std::puts("fake gtest");
#else
  std::puts("system gtest");
#endif
}
