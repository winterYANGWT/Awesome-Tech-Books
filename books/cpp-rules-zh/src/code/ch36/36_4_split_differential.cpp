// 第 36.4 节：两个独立实现做差分测试，输入覆盖空串、连续分隔符和行尾空白
// mode: run
#include <cstddef>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

// 待测实现：手写扫描，任何空白都算分隔符
std::vector<std::string> split_manual(std::string_view text) {
    std::vector<std::string> words;
    std::size_t i = 0;
    while (i < text.size()) {
        while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) {
            ++i;
        }
        const std::size_t begin = i;
        while (i < text.size() && !std::isspace(static_cast<unsigned char>(text[i]))) {
            ++i;
        }
        if (i > begin) {
            words.emplace_back(text.substr(begin, i - begin));
        }
    }
    return words;
}

// 对照实现：让标准库的输入运算符做同一件事
std::vector<std::string> split_stream(std::string_view text) {
    std::istringstream input{std::string(text)};
    std::vector<std::string> words;
    for (std::string word; input >> word;) {
        words.push_back(word);
    }
    return words;
}

struct Case {
    std::string_view input;
    std::vector<std::string> expected;
};

int main() {
    const std::vector<Case> cases{
        {"", {}},
        {"   ", {}},
        {"alpha", {"alpha"}},
        {"alpha ", {"alpha"}},
        {" alpha", {"alpha"}},
        {"alpha  beta", {"alpha", "beta"}},
        {"alpha\tbeta", {"alpha", "beta"}},
        {"  alpha   beta  ", {"alpha", "beta"}},
        {"a\nb", {"a", "b"}},
    };

    int mismatches = 0;
    int oracle_mismatches = 0;
    for (const Case& test : cases) {
        const auto manual = split_manual(test.input);
        const auto stream = split_stream(test.input);
        if (manual != stream) {
            ++mismatches;
            std::cout << "implementations differ on a case\n";
        }
        if (manual != test.expected) {
            ++oracle_mismatches;
            std::cout << "manual splitter differs from hand-checked expectation\n";
        }
    }
    std::cout << cases.size() << " cases, " << mismatches << " implementation mismatches, "
              << oracle_mismatches << " oracle mismatches\n";
    if (mismatches != 0 || oracle_mismatches != 0) {
        return 1;
    }
}
