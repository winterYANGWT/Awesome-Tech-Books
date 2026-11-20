#include <iostream>

#include "ring/ring.hpp"

int main() {
  ring::Counter counter;
  counter.add(2);
  counter.add(3);
  std::cout << "ring " << ring::version() << ": " << counter.value() << '\n';
}
