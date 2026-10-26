// 第 12.4 节：输出和返回值两个副作用，怎么排顺序才不丢结果
// mode: run
#include <cstdio>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

struct Name {
    std::string value;
    static inline bool throw_on_assign = false;

    explicit Name(std::string text) : value(std::move(text)) {}
    Name(const Name&) = default;

    Name& operator=(const Name& other) {
        if (throw_on_assign) {
            throw std::runtime_error("name assignment failed");
        }
        value = other.value;
        return *this;
    }
};

std::string full_name(bool broken) {
    if (broken) {
        throw std::runtime_error("employee record missing");
    }
    return "Ada Lovelace";
}

// 反例：消息已经输出，返回值在调用方赋值时失败，结果就丢了
Name report_by_value(std::ostream& out, bool broken) {
    const std::string name = full_name(broken);
    out << name << " is overpaid\n";
    return Name{name};  // C++17 起纯右值保证复制消除，这里的返回本身不复制
}

// 正例：先在堆上构造好结果，输出之后只做不抛的所有权转移
std::unique_ptr<Name> report_by_pointer(std::ostream& out, bool broken) {
    auto result = std::make_unique<Name>(full_name(broken));
    out << result->value << " is overpaid\n";
    return result;
}

int main() {
    Name stored{"nobody"};
    Name::throw_on_assign = true;

    std::ostringstream by_value;
    try {
        stored = report_by_value(by_value, false);
    } catch (const std::exception& e) {
        std::printf("by value failed: %s\n", e.what());
    }
    std::printf("by value: printed=[%s] stored=[%s]\n", by_value.str().c_str(),
                stored.value.c_str());

    std::ostringstream by_pointer;
    std::unique_ptr<Name> delivered;
    try {
        delivered = report_by_pointer(by_pointer, false);
    } catch (const std::exception& e) {
        std::printf("by pointer failed: %s\n", e.what());
    }
    std::printf("by pointer: printed=[%s] delivered=%d value=[%s]\n",
                by_pointer.str().c_str(), delivered ? 1 : 0,
                delivered ? delivered->value.c_str() : "");

    std::ostringstream before_output;
    try {
        (void)report_by_pointer(before_output, true);
    } catch (const std::exception& e) {
        std::printf("failure before output: %s, printed=[%s]\n", e.what(),
                    before_output.str().c_str());
    }
}
