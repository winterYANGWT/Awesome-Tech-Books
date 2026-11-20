#include <cstdio>

#include "preset_info.h"

int main() {
  std::printf("label=%s sanitize=%s build=%s\n",
              PROJ_LABEL, PROJ_SANITIZE_TEXT, PROJ_BUILD_TYPE);
}
