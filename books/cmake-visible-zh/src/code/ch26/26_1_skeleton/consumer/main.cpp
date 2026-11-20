#include <iostream>

#include "ring/ring.hpp"

int main() {
  ring::Counter counter;
  counter.add(7);
  std::cout << "consumer sees ring " << ring::version() << ": " << counter.value()
            << '\n';
}
