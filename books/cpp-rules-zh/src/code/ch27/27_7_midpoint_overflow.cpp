// 第 27.7 节：先加后除的中点会溢出，UBSan 会当场报错
// mode: crash
// expect: signed integer overflow
#include <iostream>

int midpoint_bug(int lo, int hi) {
    return (lo + hi) / 2;
}

int main() {
    int lo = 2'000'000'000;
    int hi = 2'000'000'001;
    std::cout << midpoint_bug(lo, hi) << '\n';
}
