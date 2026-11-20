#include <cstdio>

#include "configured.h"
#include "version.h"

int main() {
  std::printf("configure: %s version: %s\n", CONFIGURED_VERSION, CFG_VERSION);
}
