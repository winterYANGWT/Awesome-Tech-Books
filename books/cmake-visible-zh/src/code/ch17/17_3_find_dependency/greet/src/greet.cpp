#include "greet/greet.h"

#include <zlib.h>

#include <string>

std::size_t compressed_size(const std::string& text) {
  uLongf size = compressBound(static_cast<uLong>(text.size()));
  std::string out(size, '\0');
  const auto* in = reinterpret_cast<const Bytef*>(text.data());
  if (compress2(reinterpret_cast<Bytef*>(out.data()), &size, in,
                static_cast<uLong>(text.size()), 6) != Z_OK) {
    return 0;
  }
  return size;
}
