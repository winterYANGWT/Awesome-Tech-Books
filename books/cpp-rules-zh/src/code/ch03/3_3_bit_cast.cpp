// 第 3.3 节：std::bit_cast 把按位重解释变成有定义、可 constexpr 的操作
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>

// 位模式相同的整数与浮点数，是 union 双关最常见的动机
static_assert(sizeof(float) == sizeof(std::uint32_t));
static_assert(std::is_trivially_copyable_v<float>);

int main() {
    // C++20 起可以在常量表达式里做，编译器直接算出结果
    constexpr float one = std::bit_cast<float>(std::uint32_t{0x3f800000});
    static_assert(one == 1.0f);

    std::uint32_t bits = std::bit_cast<std::uint32_t>(1.0f);
    std::printf("bits = %04x %04x\n", bits >> 16, bits & 0xffffu);
    std::printf("%.1f\n", std::bit_cast<float>(bits));

    // C++17 及以前的等价写法：memcpy 到同宽对象，先有对象再有值
    std::uint32_t raw = 0x40490fdb;
    float pi = 0.0f;
    static_assert(sizeof pi == sizeof raw);
    std::memcpy(&pi, &raw, sizeof pi);
    std::printf("%.5f\n", pi);
}
