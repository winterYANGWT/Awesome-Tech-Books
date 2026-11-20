#include <cstdio>

int std_features();

int main() {
  std::printf("std_features=%d __cplusplus=%ld\n", std_features(), __cplusplus);
}
