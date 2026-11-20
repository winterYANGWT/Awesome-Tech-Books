#include <cstdio>

#include <libxml/parser.h>

int main() {
  xmlCheckVersion(LIBXML_VERSION);
  std::printf("libxml2=%s\n", LIBXML_DOTTED_VERSION);
}
