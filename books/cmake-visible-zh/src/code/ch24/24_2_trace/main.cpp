#include <cstdio>

int main() {
#ifdef FEATURE_LOG
  std::printf("fast=%d log=%d\n", FEATURE_FAST, FEATURE_LOG);
#else
  std::printf("fast=%d log=0\n", FEATURE_FAST);
#endif
  return 0;
}
