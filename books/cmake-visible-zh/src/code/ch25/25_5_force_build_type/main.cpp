#include <cstdio>

int main() {
#ifdef NDEBUG
  std::printf("NDEBUG=1\n");
#else
  std::printf("NDEBUG=0\n");
#endif
#ifdef __OPTIMIZE__
  std::printf("OPTIMIZE=1\n");
#else
  std::printf("OPTIMIZE=0\n");
#endif
}
