#include <cstdio>

#include "greet.h"

int main() {
  const std::string got = greeting("ci");
  std::printf("got %s\n", got.c_str());
  return got == "hello, ci" ? 0 : 1;
}
