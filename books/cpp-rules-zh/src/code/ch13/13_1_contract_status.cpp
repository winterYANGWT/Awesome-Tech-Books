// 第 13.1 节：同一个条件在 at/find/add 里分别是错误、正常结果和后置条件失败
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

class Schedule {
public:
    explicit Schedule(std::size_t capacity) : capacity_(capacity) {}

    // 前置条件：index < size()。违反时函数无法完成工作，报告错误。
    const std::string& at(std::size_t index) const {
        if (index >= sessions_.size()) {
            throw std::out_of_range(
                "Schedule::at: index " + std::to_string(index) + " out of range");
        }
        return sessions_[index];
    }

    // 找不到某个标题是合法结果，不是错误；用空指针表达“没有”。
    const std::string* find(const std::string& title) const noexcept {
        for (const auto& session : sessions_) {
            if (session == title) {
                return &session;
            }
        }
        return nullptr;
    }

    // 前置条件：title 非空。后置条件：加入后 size() <= capacity()。
    void add(std::string title) {
        if (title.empty()) {
            throw std::invalid_argument("Schedule::add: empty title");
        }
        if (sessions_.size() == capacity_) {
            throw std::length_error("Schedule::add: schedule is full");
        }
        sessions_.push_back(std::move(title));
    }

    std::size_t size() const noexcept { return sessions_.size(); }
    std::size_t capacity() const noexcept { return capacity_; }

    // 类的不变式：任何公开操作返回后都必须成立。
    bool invariant() const noexcept { return sessions_.size() <= capacity_; }

private:
    std::size_t capacity_;
    std::vector<std::string> sessions_;
};

int main() {
    Schedule schedule(2);
    schedule.add("C++ 类型系统");
    schedule.add("异常安全");

    if (const std::string* found = schedule.find("异常安全")) {
        std::cout << "find 命中：" << *found << "\n";
    }
    const std::string* missing = schedule.find("并行算法");
    std::cout << "find 未命中：返回 " << (missing ? "非空指针" : "nullptr")
              << "，调用方按正常分支处理\n";

    try {
        std::cout << schedule.at(9) << "\n";
    } catch (const std::out_of_range& e) {
        std::cout << "at 违反前置条件：" << e.what() << "\n";
    }

    try {
        schedule.add("");
    } catch (const std::invalid_argument& e) {
        std::cout << "add 拒绝空标题：" << e.what() << "\n";
    }

    try {
        schedule.add("协程");
    } catch (const std::length_error& e) {
        std::cout << "add 达不成后置条件：" << e.what() << "\n";
    }

    std::cout << "size=" << schedule.size() << " capacity=" << schedule.capacity()
              << " invariant=" << std::boolalpha << schedule.invariant() << "\n";
}
