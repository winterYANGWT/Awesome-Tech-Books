// 第 3.3 节：reinterpret_cast 不检查对齐，读一个错位的整数是未定义行为
// mode: crash
// expect: misaligned
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

int main() {
    std::uint32_t value = 0x11223344;
    // 明确按 4 字节对齐，buffer+1 就一定是错位的
    alignas(std::uint32_t) std::byte buffer[sizeof(value) + 1]{};

    // 从 buffer+1 开始放，地址是 1 mod 4，不满足 uint32_t 的 4 字节对齐
    std::memcpy(buffer + 1, &value, sizeof value);

    auto* view = reinterpret_cast<const std::uint32_t*>(buffer + 1);
    std::printf("read %08x\n", *view);
}
