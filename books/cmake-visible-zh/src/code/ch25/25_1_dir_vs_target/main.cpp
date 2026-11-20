#include <cstdio>

#include "helper.h"
#include "value.h"

int main() {
#ifdef OWNER
  const int owner = OWNER;
#else
  const int owner = -1;
#endif
  std::printf("app: %d + %d = %d, OWNER=%d\n", COMMON_VALUE, helper_value(),
              COMMON_VALUE + helper_value(), owner);
}
