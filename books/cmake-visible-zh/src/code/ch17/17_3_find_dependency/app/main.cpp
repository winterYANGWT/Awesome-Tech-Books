#include "greet/greet.h"

#include <iostream>

int main() {
  const std::string text = "CMake exports the target, zlib compresses it.";
  std::cout << "compressed " << text.size() << " bytes to " << compressed_size(text)
            << " bytes\n";
}
