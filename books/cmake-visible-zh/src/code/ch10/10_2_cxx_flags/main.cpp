#include <cstdio>

int tiny();

int main() {
#ifdef FROM_TOP_FLAGS
  std::printf("app: macro=yes tiny=%d\n", tiny());
#else
  std::printf("app: macro=no tiny=%d\n", tiny());
#endif
}
