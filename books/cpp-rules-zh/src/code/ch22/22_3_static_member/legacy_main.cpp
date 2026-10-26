#include "legacy.h"

#include <iostream>

int main() {
    const int& cap = Legacy::cap;  // odr-use
    std::cout << "Legacy::cap = " << cap << '\n';
}
