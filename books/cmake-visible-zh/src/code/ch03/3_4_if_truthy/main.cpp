#include <cstdio>

int main() {
#ifdef HAVE_EXTRA
  std::puts("HAVE_EXTRA");
#endif
#ifdef GCC15
  std::puts("GCC15");
#endif
#ifdef TWO
  std::puts("TWO");
#endif
}
