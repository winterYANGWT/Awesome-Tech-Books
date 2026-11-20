#include <cstdio>

int main(int argc, char** argv) {
  int values[3] = {10, 20, 30};
  int index = argc > 1 ? 0 : 3;
  std::printf("values[%d] = %d\n", index, values[index]);
  return 0;
}
