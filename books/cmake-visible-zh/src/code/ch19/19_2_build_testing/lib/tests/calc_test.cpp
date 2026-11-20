#include "calc.h"
#include <iostream>

int main() {
  if (add(2, 3) != 5) {
    std::cerr << "calc_test: wrong result\n";
    return 1;
  }
  std::cout << "calc_test: ok\n";
}
