#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <thread>

int main(int argc, char** argv) {
  std::string mode = argc > 1 ? argv[1] : "";
  if (mode == "fail") {
    std::fprintf(stderr, "probe: this failure is expected\n");
    return 1;
  }
  if (mode == "greet") {
    const char* who = std::getenv("GREETING");
    std::printf("hello, %s\n", who ? who : "(unset)");
    return 0;
  }
  if (mode == "sleep") {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return 0;
  }
  if (mode == "setup") {
    std::ofstream("fixture.txt") << "ready\n";
    std::puts("fixture ready");
    return 0;
  }
  if (mode == "check") {
    std::ifstream in("fixture.txt");
    std::puts(in ? "fixture present" : "fixture missing");
    return in ? 0 : 1;
  }
  if (mode == "cleanup") {
    std::remove("fixture.txt");
    std::puts("fixture removed");
    return 0;
  }
  return 2;
}
