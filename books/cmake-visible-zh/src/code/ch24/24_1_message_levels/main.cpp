#include <cstdio>
#include "config.hpp"

int work();

int main() {
  std::printf("impl=%s work=%d\n", APP_IMPL, work());
  return 0;
}
