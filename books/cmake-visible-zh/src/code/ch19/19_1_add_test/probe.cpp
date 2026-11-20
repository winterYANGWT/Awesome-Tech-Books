#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {
  if (argc > 1 && std::strcmp(argv[1], "ok") == 0) {
    return 0;
  }
  std::fprintf(stderr, "probe: this test is meant to fail\n");
  return 1;
}
