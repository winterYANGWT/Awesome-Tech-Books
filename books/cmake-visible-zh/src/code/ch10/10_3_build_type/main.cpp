#include <cstdio>

int main() {
#ifdef NDEBUG
  std::printf("NDEBUG is defined\n");
#else
  std::printf("NDEBUG is not defined\n");
#endif
}
