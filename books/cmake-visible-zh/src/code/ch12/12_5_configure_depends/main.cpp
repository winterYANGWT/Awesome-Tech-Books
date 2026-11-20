#include <cstdio>

#include "settings.h"
#include "template.h"

int main() {
  std::printf("settings: %s\n", SETTINGS_VALUE);
  std::printf("template: %s\n", TEMPLATE_VALUE);
}
