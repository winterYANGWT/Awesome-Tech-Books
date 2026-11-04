// mode: fail
// std: c++23
// expect: pointer_safety
#include <memory>

int main() {
    std::pointer_safety s = std::pointer_safety::relaxed;
    (void)s;
}
