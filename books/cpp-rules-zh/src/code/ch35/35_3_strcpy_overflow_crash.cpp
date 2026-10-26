// 第 35.3 节：strcpy 不检查目标缓冲区大小
// mode: crash
// expect: stack-buffer-overflow
#include <cstring>
#include <iostream>

#if defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
void copy_label(char* dst, const char* src) {
    std::strcpy(dst, src);
}

int main(int argc, char**) {
    char label[8];
    std::cout << "copying a label into 8 bytes\n";
    copy_label(label, argc > 0 ? "warehouse-7" : "x");
    std::cout << label << '\n';
}
