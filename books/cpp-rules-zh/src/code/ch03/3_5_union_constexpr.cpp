// 第 3.5 节：常量求值中读 union 的非活跃成员，编译器直接拒绝
// mode: fail
// expect: active member
#include <cstdint>

union Bits {
    std::uint32_t integer;
    float real;
};

constexpr float reinterpret_bits(std::uint32_t value) {
    Bits bits{};
    bits.integer = value;
    return bits.real;   // 活跃成员是 integer，读 real 是未定义行为
}

constexpr float one = reinterpret_bits(0x3f800000u);
static_assert(one == 1.0f);

int main() {
    return 0;
}
