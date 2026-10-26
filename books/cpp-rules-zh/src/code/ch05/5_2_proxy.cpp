// 第 5.2 节：auto 会把 vector<bool> 的代理对象原样存下来
#include <bitset>
#include <cstddef>
#include <iostream>
#include <type_traits>
#include <vector>

std::vector<bool> feature_flags() {
    return {false, false, true, false, true, false, false, true};
}

int main() {
    std::vector<bool> flags = feature_flags();

    // vector<bool> 按位存储，operator[] 返回代理类 reference，不是 bool&
    static_assert(std::is_same_v<decltype(flags[2]), std::vector<bool>::reference>);
    static_assert(!std::is_same_v<decltype(flags[2]), bool&>);

    auto proxy = flags[2];       // 代理对象，仍指向 flags 里的那一位
    static_assert(std::is_same_v<decltype(proxy), std::vector<bool>::reference>);
    std::cout << "proxy = " << proxy << "\n";

    proxy = false;               // 通过代理写回容器
    std::cout << "写回后 flags[2] = " << flags[2] << "\n";

    // 用显式类型或显式类型初始化器，得到真正的 bool 值
    bool value = flags[4];
    auto corrected = static_cast<bool>(flags[4]);
    static_assert(std::is_same_v<decltype(corrected), bool>);
    std::cout << "value = " << value << "，corrected = " << corrected << "\n";

    // bitset 的 operator[] 同样返回代理
    std::bitset<8> bits(0b1010'0000);
    static_assert(std::is_same_v<decltype(bits[5]), std::bitset<8>::reference>);
    auto bit = bits[5];
    bool bit_value = bits[5];
    std::cout << "bit = " << bit << "，bit_value = " << bit_value << "\n";
}
