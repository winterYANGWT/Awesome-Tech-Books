// 第 24.5 节：vector<bool> 存的是位，operator[] 返回代理而不是 bool&
#include <bitset>
#include <concepts>
#include <deque>
#include <iostream>
#include <ranges>
#include <span>
#include <type_traits>
#include <vector>

int main() {
    std::vector<bool> flags(8, false);
    flags[2] = true;

    // operator[] 返回的是代理对象，不是 bool 的引用
    static_assert(std::is_same_v<decltype(flags[0]), std::vector<bool>::reference>);
    static_assert(!std::is_same_v<decltype(flags[0]), bool&>);

    // 因此它不能把 &v[0] 当成 bool*，std::span 也接不上它
    static_assert(!std::is_constructible_v<std::span<bool>, std::vector<bool>&>);

    // ranges 的 contiguous_range 也直接给出否定答案
    static_assert(!std::ranges::contiguous_range<std::vector<bool>>);
    static_assert(std::ranges::contiguous_range<std::vector<char>>);

    std::cout << std::boolalpha;

    // 代理指向那一位，像引用一样跟着容器变；bool 变量是当时的快照
    auto proxy = flags[2];
    const bool snapshot = flags[2];
    flags[2] = false;
    std::cout << "改掉第 2 位后，代理读到 " << static_cast<bool>(proxy)
              << "，快照仍是 " << snapshot << '\n';

    // 需要真正的 bool 容器时用 deque<bool>，它的 operator[] 返回 bool&
    std::deque<bool> real{true, true, true, true};
    real[0] = false;
    bool* first = &real[0];
    *first = true;
    static_assert(std::is_same_v<decltype(real[0]), bool&>);
    std::cout << "deque<bool> 第一个元素: " << real[0] << '\n';

    // 长度固定、需要位操作时用 bitset：没有迭代器，但能 flip/count
    std::bitset<8> bits;
    bits.set(2);
    bits.flip(0);
    std::cout << "bitset: " << bits << "，置位个数 " << bits.count() << '\n';

    // 只想要一块可写的字节标志时，vector<char> 是普通容器
    std::vector<char> bytes(4, 0);
    char* raw = bytes.data();
    raw[1] = 1;
    std::cout << "vector<char> 第 1 字节: " << static_cast<int>(bytes[1]) << '\n';
}
