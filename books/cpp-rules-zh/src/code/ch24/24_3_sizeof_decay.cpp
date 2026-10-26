// 第 24.3 节：数组退化成指针后，sizeof 除法算的是指针大小
// mode: warn
// expect: -Wsizeof-pointer-div
#include <cstddef>
#include <iostream>

// 形参写成指针，数组已经退化；sizeof(p) 是指针大小，不是整个数组
void inspect_pointer(const int* p) {
    std::cout << "指针形参里 sizeof(p)/sizeof(*p) = " << sizeof(p) / sizeof(*p) << '\n';
}

int main() {
    int grades[8]{90, 85, 77, 92, 60, 88, 95, 71};
    inspect_pointer(grades);
}
