#include "budget.h"

#include <iostream>

int main() {
    const int& limit = Budget::limit;  // odr-use：必须能找到类外定义
    const int& cap = Budget::cap;      // C++17 起 cap 是 inline 变量，不必类外定义
    std::cout << "limit = " << limit << ", cap = " << cap
              << ", rate = " << Budget::rate << '\n';
}
