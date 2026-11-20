#include <cstdio>

#ifndef MODE
#define MODE "unset"
#endif
#ifndef LABEL
#define LABEL "?"
#endif

int main() {
  std::printf("%s: MODE=%s\n", LABEL, MODE);
}
