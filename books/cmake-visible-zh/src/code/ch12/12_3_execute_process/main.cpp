#include <cstdio>

#include "version.h"

int main() {
  std::printf("compiler: %s\n", COMPILER_LINE);
  std::printf("tool: %s\n", TOOL_VERSION);
}
