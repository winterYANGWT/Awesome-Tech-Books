#include <iostream>
#include "gui.h"
#include "net.h"

int main() {
  std::cout << net_value() + gui_value() << "\n";
  return 0;
}
