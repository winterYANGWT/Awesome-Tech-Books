// 第 13.1 节：查找未命中在 lookup 里是正常结果，在 require 里才成为错误
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>

struct MissingSetting : std::runtime_error {
    explicit MissingSetting(const std::string& key)
        : std::runtime_error("required setting missing: " + key), key_(key) {}

    const std::string& key() const noexcept { return key_; }

private:
    std::string key_;
};

class Settings {
public:
    void set(std::string key, std::string value) {
        values_[std::move(key)] = std::move(value);
    }

    // 前置条件只有“key 是合法字符串”。没找到是合法结果：
    // 调用方可能正想用默认值，这里没有足够的上下文判定它是错误。
    std::optional<std::string> lookup(const std::string& key) const {
        const auto it = values_.find(key);
        if (it == values_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    // 这个函数的契约要求 key 一定存在，它自己的后置条件包含这一点。
    // 同一个条件到了这一层就是错误，因为函数拿不出返回值。
    std::string require(const std::string& key) const {
        if (auto value = lookup(key)) {
            return *value;
        }
        throw MissingSetting(key);
    }

private:
    std::map<std::string, std::string> values_;
};

int main() {
    Settings settings;
    settings.set("host", "localhost");

    const auto port = settings.lookup("port");
    std::cout << "lookup(\"port\")："
              << (port ? *port : std::string("<没有值>"))
              << "，这是正常结果，没有异常\n";

    try {
        std::cout << settings.require("port") << "\n";
    } catch (const MissingSetting& e) {
        std::cout << "require(\"port\") 报告错误：" << e.what()
                  << "（缺少的键是 " << e.key() << "）\n";
    }

    std::cout << "require(\"host\")：" << settings.require("host") << "\n";
}
