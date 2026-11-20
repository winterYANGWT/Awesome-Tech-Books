#include <iostream>
#include "config.h"

int main() {
  std::cout << APP_VERSION << " " << APP_CODENAME << " " << BUILD_ID << "\n";
  return 0;
}
