#include <cstdio>

#include "config.h"

int main() {
  std::printf("app=%s version=%s\n", APP_NAME, APP_VERSION);
#ifdef HAVE_OPENSSL
  std::puts("openssl: on");
#else
  std::puts("openssl: off");
#endif
#if ENABLE_LOGGING
  std::puts("logging: on");
#else
  std::puts("logging: off");
#endif
}
