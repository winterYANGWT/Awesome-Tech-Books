// 第 35.4 节：数组形参退化成指针，sizeof 不再是数组大小
// mode: warn
// expect: -Wsizeof-array-argument
#include <iostream>

double average_legacy(const double values[8]) {
    std::cout << "inside sizeof=" << sizeof(values) << '\n';
    double total = 0;
    for (int i = 0; i < 8; ++i) {
        total += values[i];
    }
    return total / 8;
}

int main() {
    double samples[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    std::cout << "caller sizeof=" << sizeof(samples) << '\n';
    std::cout << "average=" << average_legacy(samples) << '\n';
}
