// 第 13.3 节：消息存成成员，what() 返回的指针跟随异常对象存活
#include <exception>
#include <iostream>
#include <string>

class ConfigError : public std::exception {
public:
    ConfigError(std::string key, std::string detail)
        : message_("config error for '" + key + "': " + detail) {}

    // 返回成员 string 的缓冲区。异常对象活多久，这块缓冲区就活多久。
    const char* what() const noexcept override { return message_.c_str(); }

    void add_retry_hint() { message_ += " (retry with a numeric value)"; }

private:
    std::string message_;
};

int main() {
    try {
        throw ConfigError("db.port", "not a number");
    } catch (const ConfigError& e) {
        const char* original = e.what();
        std::cout << "原始异常：" << original << "\n";

        // 复制的是另一个异常对象，消息也复制了一份；改副本不影响原对象。
        ConfigError copy = e;
        copy.add_retry_hint();
        std::cout << "副本：" << copy.what() << "\n";
        std::cout << "原对象仍然有效：" << original << "\n";
    }

    // 标准库的 runtime_error 已经替我们管理好消息的生命期。
    try {
        throw std::runtime_error("cannot open /etc/app.conf: permission denied");
    } catch (const std::exception& e) {
        std::cout << "runtime_error：" << e.what() << "\n";
    }
}
