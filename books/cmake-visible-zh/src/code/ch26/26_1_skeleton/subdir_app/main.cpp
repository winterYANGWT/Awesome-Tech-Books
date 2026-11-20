#include <iostream>

#include "ring/ring.hpp"

int main() {
  std::cout << "subdir app sees ring " << ring::version() << ": "
            << ring::sum({2, 3, 4}) << '\n';
}
