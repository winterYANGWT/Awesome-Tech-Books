// 第 15.2 节：basic_string::copy 不检查目标缓冲区大小，也不补 '\0'
// mode: crash
// expect: stack-buffer-overflow
#include <iostream>
#include <string>

int main() {
    std::string serial = "SN-2026-0042";
    char buffer[8];  // 只够 7 个字符加结尾 '\0'

    std::cout << "buffer 8 字节，serial " << serial.size() << " 字节\n";

    // copy 不检查 buffer 的大小，直接把 12 个字符写进去。
    auto copied = serial.copy(buffer, serial.size());
    buffer[copied] = '\0';  // 再越界写一个字节

    std::cout << buffer << '\n';
}
