#pragma once

#include <string>

#include "legacy_formatter.h"

// 用私有继承复用实现：客户端包含这个头文件时，也被迫包含 legacy_formatter.h。
class InheritedReport : private LegacyFormatter {
public:
    std::string render(const std::string& text) const;
};
