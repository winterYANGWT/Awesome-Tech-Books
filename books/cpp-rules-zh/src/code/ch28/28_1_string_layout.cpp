// 第 28.1 节：观察 string 对象大小、容量与短串优化，说明这些都是实现细节
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

int main() {
    std::cout << "sizeof(std::string)      = " << sizeof(std::string) << '\n';
    std::cout << "sizeof(std::string_view) = " << sizeof(std::string_view) << '\n';

    std::string empty;
    std::string shortName = "Perse";
    std::string longName(40, 'x');

    auto inside_object = [](const std::string& s) {
        const auto begin = reinterpret_cast<std::uintptr_t>(&s);
        const auto data = reinterpret_cast<std::uintptr_t>(s.data());
        return data >= begin && data < begin + sizeof(s);
    };

    std::cout << "空串: size=" << empty.size() << " capacity=" << empty.capacity()
              << " 缓冲区在对象内=" << (inside_object(empty) ? "是" : "否") << '\n';
    std::cout << "短串: size=" << shortName.size() << " capacity=" << shortName.capacity()
              << " 缓冲区在对象内=" << (inside_object(shortName) ? "是" : "否") << '\n';
    std::cout << "长串: size=" << longName.size() << " capacity=" << longName.capacity()
              << " 缓冲区在对象内=" << (inside_object(longName) ? "是" : "否") << '\n';

    // 下面这组数字只描述本机的 libstdc++：标准没有规定扩容因子，
    // 换一个标准库（例如 libc++）或版本，序列就会变。
    std::string growing;
    std::size_t last = growing.capacity();
    std::cout << "本机 string 逐字符追加时的容量变化: " << last;
    for (int i = 0; i < 80; ++i) {
        growing.push_back('a');
        if (growing.capacity() != last) {
            last = growing.capacity();
            std::cout << " -> " << last;
        }
    }
    std::cout << '\n';
}
