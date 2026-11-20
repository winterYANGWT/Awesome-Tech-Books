#include "iface.h"
#include <cstdio>

int stat_value();
int shared_value();
int obj_value();

int main() {
  std::printf("stat=%d shared=%d obj=%d iface=%d\n",
              stat_value(), shared_value(), obj_value(), iface_value());
}
