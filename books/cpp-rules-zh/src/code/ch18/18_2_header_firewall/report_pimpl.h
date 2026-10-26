#pragma once

#include <memory>
#include <string>

// 只前置声明实现，客户端不需要看见 LegacyFormatter。
class PimplReport {
public:
    PimplReport();
    ~PimplReport();

    std::string render(const std::string& text) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
