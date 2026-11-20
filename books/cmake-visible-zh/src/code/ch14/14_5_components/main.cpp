#include <boost/filesystem.hpp>

#include <iostream>

int main() {
  boost::filesystem::path p("/tmp/book/ch14.txt");
  std::cout << p.filename().string() << " root_exists=" << boost::filesystem::exists("/") << '\n';
}
