#include <cstdio>

int classify(int x) {
  if (x > 0) return 1;
  if (x < 0) return -1;
  return 0;
}

int main() {
  int sum = 0;
  for (int i = 1; i <= 4; ++i) sum += i;
  std::printf("sum=%d class=%d\n", sum, classify(sum));
  return sum == 10 ? 0 : 1;
}
