#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

// 头文件自己包含它声明所需的一切，使用者不必先猜该包含什么。
class AuditLog {
public:
    void add(std::string message) { messages_.push_back(std::move(message)); }
    std::size_t size() const { return messages_.size(); }

private:
    std::vector<std::string> messages_;
};
