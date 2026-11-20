#include <cstdio>
#include <zlib.h>

int main() {
  std::printf("zlib=%s\n", zlibVersion());
  return 0;
}
