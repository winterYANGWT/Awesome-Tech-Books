#pragma once

// 这段声明用到了 string、vector、size_t 和 move，却没有包含任何头文件。
// 它能被编译，只是因为包含它的 .cpp 碰巧先包含了别的头文件。
class AuditLog {
public:
    void add(std::string message) { messages_.push_back(std::move(message)); }
    std::size_t size() const { return messages_.size(); }

private:
    std::vector<std::string> messages_;
};
