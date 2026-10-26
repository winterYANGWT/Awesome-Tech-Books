// 第 14.6 节：没有合理零值的类型不提供默认构造
#include <cstddef>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class AccountId {
public:
    explicit AccountId(std::string code) : code_(std::move(code)) {
        if (code_.size() != 5 || code_[0] != 'A' || code_[1] != '-') {
            throw std::invalid_argument("bad account id");
        }
    }

    [[nodiscard]] const std::string& code() const { return code_; }

private:
    std::string code_;
};
static_assert(!std::is_default_constructible_v<AccountId>);

// 反面：用空字符串当“未指定”的哨兵，每个使用者都要记得检查。
class LegacyAccountId {
public:
    LegacyAccountId(std::string code = {}) : code_(std::move(code)) {}
    [[nodiscard]] bool valid() const { return code_.size() == 5 && code_[0] == 'A' && code_[1] == '-'; }
    [[nodiscard]] const std::string& code() const { return code_; }

private:
    std::string code_;
};
static_assert(std::is_default_constructible_v<LegacyAccountId>);

int main() {
    // 现代容器不要求元素可默认构造：reserve + emplace_back 就够。
    std::vector<AccountId> ids;
    ids.reserve(2);
    ids.emplace_back("A-100");
    ids.push_back(AccountId("A-200"));

    // “还没有 ID”用 optional 显式表达，而不是造一个空 AccountId。
    std::optional<AccountId> pending;
    pending.emplace("A-300");

    std::map<std::string, AccountId> byOwner;
    byOwner.emplace("carol", AccountId("A-400"));

    for (const AccountId& id : ids) std::cout << id.code() << ' ';
    std::cout << "pending=" << pending->code()
              << " map=" << byOwner.at("carol").code() << "\n";

    LegacyAccountId unset;   // 半成品：编译通过，但内容无效
    int checks = 0;
    auto use = [&checks](const LegacyAccountId& id) {
        ++checks;
        return id.valid() ? id.code() : std::string("<unset>");
    };
    std::cout << "unset=" << use(unset) << " checks=" << checks << "\n";
    std::cout << "set=" << use(LegacyAccountId("A-500")) << " checks=" << checks << "\n";
}
