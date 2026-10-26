// 第 3.2 节：标准对布局保证到哪一步，以及 char 到底存了什么
#include <climits>
#include <cstddef>
#include <cstdio>
#include <limits>
#include <type_traits>

struct Packet {
    char kind;
    int sequence;
    char payload;
};

// 标准布局：所有非静态数据成员同一访问控制、没有虚函数等
static_assert(std::is_standard_layout_v<Packet>);

class Split {
public:
    int first;
private:
    [[maybe_unused]] int second;
public:
    int third;
};

// 访问控制块被切开，就不再是标准布局
static_assert(!std::is_standard_layout_v<Split>);

struct alignas(16) Vec4 {
    float x, y, z, w;
};
static_assert(alignof(Vec4) == 16);

int main() {
    std::printf("sizeof(Packet)=%zu alignof=%zu\n",
                sizeof(Packet), alignof(Packet));
    std::printf("offsetof: kind=%zu sequence=%zu payload=%zu\n",
                offsetof(Packet, kind), offsetof(Packet, sequence),
                offsetof(Packet, payload));
    std::printf("standard layout: Packet=%d Split=%d\n",
                std::is_standard_layout_v<Packet>,
                std::is_standard_layout_v<Split>);

    std::printf("CHAR_BIT=%d sizeof(char)=%zu\n", CHAR_BIT, sizeof(char));
    std::printf("plain char signed=%d; (char)0x80 -> %d\n",
                std::numeric_limits<char>::is_signed,
                static_cast<int>(static_cast<char>(0x80)));

    unsigned char uc = 0x80;
    std::printf("unsigned char 0x80 -> %u\n", static_cast<unsigned>(uc));

    // std::byte 是独立的类型：只能按位运算，不会像 char 那样参与算术
    std::byte b{0x80};
    std::printf("std::byte 0x80 -> %u\n", std::to_integer<unsigned>(b));
}
