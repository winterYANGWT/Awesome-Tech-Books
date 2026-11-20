#include <cstdio>
#include <string>

std::string greeting();

int main() {
  std::printf("%s\n", greeting().c_str());
  return 0;
}
