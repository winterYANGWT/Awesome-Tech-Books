// 第 7.4 节：拷贝/移动构造的副作用不能承担注册、计数或资源管理
#include <iostream>
#include <string>
#include <utility>
#include <vector>

struct Account {
    static std::vector<const Account*> registry;
    std::string owner;

    explicit Account(std::string name) : owner(std::move(name)) {}

    Account(const Account& other) : owner(other.owner) {
        registry.push_back(this);
    }

    Account(Account&& other) noexcept : owner(std::move(other.owner)) {
        registry.push_back(this);
    }
};
std::vector<const Account*> Account::registry;

Account open_account(std::string owner) {
    return Account(std::move(owner));  // 纯右值
}

struct FixedAccount {
    static int registered;
    std::string owner;

    explicit FixedAccount(std::string name) : owner(std::move(name)) {
        ++registered;  // 注册放在普通构造里，不依赖拷贝/移动是否被调用
    }

    FixedAccount(const FixedAccount&) = default;
    FixedAccount(FixedAccount&&) noexcept = default;

    FixedAccount clone() const {
        return FixedAccount(owner);  // 显式的“新建一个账户”
    }
};
int FixedAccount::registered = 0;

FixedAccount open_fixed(std::string owner) {
    return FixedAccount(std::move(owner));
}

int main() {
    Account first = open_account("Ada");
    std::cout << "创建后 registry = " << Account::registry.size()
              << "（对象已经存在）\n";

    Account second = first;
    std::cout << "复制后 registry = " << Account::registry.size() << "\n";

    FixedAccount fixed = open_fixed("Grace");
    std::cout << "FixedAccount 创建后 registered = " << FixedAccount::registered << "\n";

    FixedAccount copy = fixed.clone();
    std::cout << "clone 后 registered = " << FixedAccount::registered
              << "，owner = " << copy.owner << "\n";

    std::cout << "两个原始账户：" << first.owner << " / " << second.owner << "\n";
}
