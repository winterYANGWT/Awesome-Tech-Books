// 一个没有虚函数、只有实现细节的老类。
#pragma once

#include <string>

class LegacyFormatter {
public:
    std::string format(const std::string& text) const { return "[" + text + "]"; }
};
