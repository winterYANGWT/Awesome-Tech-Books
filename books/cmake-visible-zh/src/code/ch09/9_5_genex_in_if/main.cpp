#include <iostream>

int main() {
#ifdef DEBUG_BRANCH
  std::cout << "branch=yes\n";
#else
  std::cout << "branch=no\n";
#endif
}
