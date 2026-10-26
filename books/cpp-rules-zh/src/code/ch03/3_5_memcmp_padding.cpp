// 第 3.5 节：memcmp 比较的是字节，填充字节不属于对象的值
#include <cstdio>
#include <cstring>
#include <type_traits>

struct Sample {
    char code;
    int value;
};

// 可平凡复制只说明“可以按字节搬”，不说明“字节就是值”
static_assert(std::is_trivially_copyable_v<Sample>);
// 成员之间确实有填充，memcmp 会比到值以外的字节
static_assert(sizeof(Sample) > sizeof(char) + sizeof(int));

int main() {
    Sample a{}, b{};

    // 让两个对象的填充字节不同，模拟它们由不同路径构造后的内存
    std::memset(&a, 0xAA, sizeof a);
    a.code = 'x';
    a.value = 42;

    std::memset(&b, 0x00, sizeof b);
    b.code = 'x';
    b.value = 42;

    bool members_equal = a.code == b.code && a.value == b.value;
    std::printf("members equal: %d\n", members_equal);
    std::printf("memcmp result: %d\n", std::memcmp(&a, &b, sizeof a));
}
